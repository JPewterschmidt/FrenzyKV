#include "frenzykv/persistent/block.h"
#include "frenzykv/util/comp.h"
#include "frenzykv/util/serialize_helper.h"
#include <cstring>
#include <cassert>
#include <iterator>
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

    while (!consumed_completely(current))
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

bool block_segment::fit_public_prefix(const_bspan user_prefix) const noexcept
{
    if (user_prefix.size() > m_prefix.size()) 
        return false;

    return memcmp_comparator{}(
        user_prefix, m_prefix.subspan(0, user_prefix.size())
    ) == ::std::strong_ordering::equal;
}

/*
 *  |                         Block                        |
 *  |-----|----|----|----|------|------|------|-----|------|
 *  | BTL | BS | BS | BS | .... | SBSO | SBSO | ... | NSBS |
 *  |-----|----|----|----|------|------|------|-----|------|
 *  |     |    Data             |  Meta Data               |
 *                              ^                   ^
 *                              |                   |
 *                              |                   nsbs_beg_ptr(storage)
 *                              meta_data_beg_ptr(storage)
 *  
 *  BTL:    8   uint64_t    Block total length
 *  SBSO:   4   uint32_t    Special Block Segment Offset
 *  NSBS:   2   uint16_t    Number of Special Block Segment
 */

using btl_t = uint64_t;
using nsbs_t = uint16_t;
using sbso_t = uint32_t;
static constexpr size_t bs_bl = sizeof(btl_t);

static const ::std::byte* nsbs_beg_ptr(const_bspan s)
{
    return s.data() + s.size() - sizeof(nsbs_t);
}

static nsbs_t nsbs_value(const_bspan s)
{
    nsbs_t result{};
    const auto* p = nsbs_beg_ptr(s);
    ::std::memcpy(&result, p, sizeof(nsbs_t));

    return result;
}

static sbso_t sbso_value(const ::std::byte* sbso_ptr)
{
    sbso_t result{};
    ::std::memcpy(&result, sbso_ptr, sizeof(sbso_t));
    return result;
}

static const ::std::byte* meta_data_beg_ptr(const_bspan s)
{
    nsbs_t nsbs = nsbs_value(s);
    return nsbs_beg_ptr(s) - (nsbs * sizeof(sbso_t));
}

parse_result_t block::parse_meta_data()
{
    const ::std::byte* sbso_cur = meta_data_beg_ptr(m_storage);
    const ::std::byte* sbso_sentinal = nsbs_beg_ptr(m_storage);
    const auto nsbs = nsbs_value(m_storage);
    if (nsbs == 0) return parse_result_t::error;
    while (sbso_cur < sbso_sentinal)
    {
        m_special_segs.push_back(m_storage.data() + sbso_value(sbso_cur));
        sbso_cur += sizeof(sbso_t);
    }
    assert(m_special_segs.size() == nsbs);

    return parse_result_t::success;
}

koios::generator<block_segment> block::
segments(::std::vector<const ::std::byte*>::const_iterator insert_iter)
{
    // TODO: untested
    const ::std::byte* from = *insert_iter;
    const ::std::byte* current = from;
    
    auto parsing_end_iter = ::std::next(insert_iter);

    const ::std::byte* const sentinal = 
        (parsing_end_iter == m_special_segs.end()) ? meta_data_beg_ptr(m_storage) : *parsing_end_iter;

    while (current < sentinal)
    {
        block_segment seg{ { current, static_cast<size_t>(sentinal - current) } };
        if (const auto pr = seg.parse_result(); 
            pr != parse_result_t::success)
        {
            break;
        }
        auto seg_storage = seg.storage();
        current += seg_storage.size();

        // We could not insert the newly parsed seg position in 
        // m_storage to m_special_segs.
        //
        // Like a skip_list, elements of m_special_segs should keep 
        // their distance far enough to ensure the searching performance.
        co_yield ::std::move(seg);
    }
}

block_segment_builder::
block_segment_builder(::std::string& dst, ::std::string_view userkey) noexcept
    : m_storage{ dst }, m_userkey{ userkey }
{
    // Serialize PPL and PP
    ppl_t ppl = static_cast<ppl_t>(userkey.size());
    append_encode_int_to<sizeof(ppl)>(ppl, m_storage);
    m_storage.append(userkey);
}

bool block_segment_builder::add(const kv_entry& kv)
{
    if (m_finish) [[unlikely]] return false;

    if (kv.key().user_key() != m_userkey)
        return false;

    // Serialize RIL and RI
    ril_t ril = (ril_t)(sizeof(sequence_number_t) + kv.value().serialized_bytes_size());
    append_encode_int_to<sizeof(ril)>(ril, m_storage);
    kv.value().serialize_append_to_string(m_storage);

    return true;
}   
       
void block_segment_builder::finish()
{
    if (m_finish) [[unlikely]] return;
    ::std::array<char, 4> buffer{};
    m_storage.append(buffer.data(), buffer.size());
    m_finish = true;
}

block_builder::block_builder(const kvdb_deps& deps) 
    : m_deps{ &deps }
{
    // Prepare BTL space
    m_storage.resize(sizeof(btl_t), 0);
}

void block_builder::add(const kv_entry& kv)
{
    const size_t interval_sz = m_deps->opt()->max_block_segments_number;

    if (!m_current_seg_builder)
    {
        m_current_seg_builder = ::std::make_unique<block_segment_builder>(m_storage, kv.key().user_key());
        m_sbsos.push_back(sizeof(btl_t));
    }

    // Segment end, need a new segment.
    if (!m_current_seg_builder->add(kv))
    {
        m_current_seg_builder->finish();
        ++m_seg_count;
        if ((m_seg_count + 1) % interval_sz == 0)
        {
            m_sbsos.push_back(static_cast<sbso_t>(m_storage.size()));
            assert(m_storage[m_storage.size() - 1] == 0);
            assert(m_storage[m_storage.size() - 2] == 0);
            assert(m_storage[m_storage.size() - 3] == 0);
            assert(m_storage[m_storage.size() - 4] == 0);
        }
        m_current_seg_builder = ::std::make_unique<block_segment_builder>(m_storage, kv.key().user_key());
        [[maybe_unused]] bool add_result = m_current_seg_builder->add(kv);
        assert(add_result);
    }
}

::std::string block_builder::finish()
{
    // If the last sbso point to exactly the end of data area, then delete it.
    if (m_sbsos.back() == m_storage.size())
    {
        m_sbsos.erase(::std::prev(m_sbsos.end()));
    }

    // Serialize Meta data area.
    for (sbso_t sbso : m_sbsos)
    {
        append_encode_int_to<sizeof(sbso_t)>(sbso, m_storage);
    }
    append_encode_int_to<sizeof(nsbs_t)>(static_cast<nsbs_t>(m_sbsos.size()), m_storage);
    
    // Serialize BTL
    btl_t btl = m_storage.size();
    ::std::array<char, sizeof(btl)> buffer{};
    encode_int_to<sizeof(btl_t)>(btl, buffer);

    for (size_t i{}; i < sizeof(btl); ++i)
    {
        m_storage[i] = buffer[i];
    }

    return ::std::move(m_storage);
}

} // namespace frenzykv
