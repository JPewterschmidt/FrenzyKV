#ifndef FRENZYKV_BLOCK_H
#define FRENZYKV_BLOCK_H

#include <string_view>
#include <vector>
#include <cstdint>
#include "koios/exceptions.h"
#include "frenzykv/util/parse_result.h"
#include "frenzykv/types.h"

namespace frenzykv
{

class block_segment
{
public:
    block_segment(const_bspan block_seg_storage)
        : m_storage{ block_seg_storage }
    {
        if ((m_parse_result = parse()) == parse_result_t::error) 
            throw koios::exception{"block_segment: parse fail"};
    }

    /*! \brief  determine the parse situation.
     *  \attention The content retrived by this class could be used only this functiuon return `parse_result_t::success`
     */
    parse_result_t parse_result() const noexcept { return m_parse_result; }

    const_bspan public_prefix() const noexcept { return m_prefix; }
    size_t count() const noexcept { return m_items.size(); }
    const auto& items() const noexcept { return m_items; }
    size_t storage_bytes_size() const noexcept { return m_storage.size(); }

private:
    parse_result_t parse();

private:
    const_bspan m_storage;
    const_bspan m_prefix;
    ::std::vector<const_bspan> m_items;
    parse_result_t m_parse_result;
};

class block
{
public:
    block(const_bspan block_storage)
        : m_storage{ block_storage }
    {
        if ((m_parse_result = parse()) == parse_result_t::error)
            throw koios::exception{"block: parse fail"};
    }
    
private:
    parse_result_t parse();

private:
    const_bspan m_storage;
    ::std::vector<block_segment> m_segments;
    parse_result_t m_parse_result{};
};

} // namespace frenzykv

#endif
