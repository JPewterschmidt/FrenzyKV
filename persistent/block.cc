#include "frenzykv/persistent/block.h"
#include <cstring>
#include <cassert>
#include <array>

namespace frenzykv
{

/*  block segment format
 *
 *  PP:         string      public prefix
 *  PPL:    2B  uint16_t    public prefix length
 *  RIL:    4B  uint32_t    rest item length
 *  RI:         string      rest item
 *
 *  | PPL |      PP      | RIL |      RI       | RIL |     RI     | ... | 4B Zero EOF (RIL) |
 *
 *
 */

using ppl_t = uint16_t;
using ril_t = uint32_t;
static constexpr size_t bs_ppl = sizeof(ppl_t);
static constexpr size_t bs_ril = sizeof(ril_t);

static ril_t read_ril(const ::std::byte* cur)
{
    ril_t result{};
    ::std::memcpy(&result, cur, bs_ril);
    return result;
}

template<::std::size_t Len>
static bool filled_with_zero(const ::std::byte* p)
{
    static ::std::array<::std::byte, Len> example{};
    int ret = ::std::memcmp(p, example.data(), Len);
    return ret == 0;
}

parse_result_t block_segment::parse()
{
    const ::std::byte* current = m_storage.data();
    const ::std::byte* sentinal = current + m_storage.size();

    ppl_t ppl{};
    ::std::memcpy(&ppl, current, bs_ppl);
    if (ppl == 0) return parse_result_t::error;
    current += bs_ppl;
    
    m_prefix = { current, ppl };
    current += ppl;
    
    auto consumed_completely = [sentinal](const ::std::byte* cur){ 
        if (cur >= sentinal) return true;
        return filled_with_zero<bs_ril>(cur);
    };

    while (consumed_completely(current))
    {
        ril_t r = read_ril(current);
        current += bs_ril;
        m_items.emplace_back(current, r);   
        current += r;
    }

    if (current > sentinal) 
        return parse_result_t::partial;
    else if (filled_with_zero<bs_ril>(current))
    {
        m_storage = m_storage.subspan(0, current - m_storage.data());
    }

    return parse_result_t::success;
}

/*
 *  | BTL | BS | BS | BS | ....
 *  
 *  BTL:    8   uint64_t    Block total length
 */

using btl_t = uint64_t;
static constexpr size_t bs_bl = sizeof(btl_t);

parse_result_t block::parse()
{
    btl_t total_len{};
    ::std::memcpy(&total_len, m_storage.data(), bs_bl);
    if (total_len == 0) return parse_result_t::error;
    m_storage = m_storage.subspan(0, total_len);
    
    const ::std::byte* current = m_storage.data();
    const ::std::byte* sentinal = current + m_storage.size();
    
    while (current < sentinal)
    {
        block_segment seg{ { current, static_cast<size_t>(sentinal - current) } };
        if (parse_result_t pr = seg.parse_result(); 
            pr != parse_result_t::success)
        {
            return pr;
        }
        current += seg.storage_bytes_size();
        m_segments.push_back(::std::move(seg));
    }

    return parse_result_t::success;
}

} // namespace frenzykv
