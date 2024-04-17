#include "frenzykv/persistent/block.h"
#include <cstring>

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

static ril_t read_ril(const char* cur)
{
    ril_t result{};
    ::std::memcpy(&result, cur, bs_ril);
    return result;
}

parse_result_t block_segment::parse()
{
    const char* current = m_storage.data();
    const char* sentinal = current + m_storage.size();

    ppl_t ppl{};
    ::std::memcpy(&ppl, current, bs_ppl);
    current += bs_ppl;
    
    m_prefix = { current, ppl };
    current += ppl;
    
    auto consumed_completely = [sentinal](const char* cur){ 
        static ::std::string eof_example(4, 0);
        if (cur >= sentinal) return true;
        return ::std::string_view{ cur, 4 } == eof_example;
    };

    while (consumed_completely(current))
    {
        ril_t r = read_ril(current);
        current += bs_ril;
        m_items.emplace_back(current, r);   
        current += r;
    }

    if (current >= sentinal) 
        return parse_result_t::partial;

    return parse_result_t::success;
}

} // namespace frenzykv
