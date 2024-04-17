#ifndef FRENZYKV_BLOCK_H
#define FRENZYKV_BLOCK_H

#include <string_view>
#include <vector>
#include <cstdint>
#include "koios/exceptions.h"
#include "frenzykv/util/parse_result.h"

namespace frenzykv
{

class block_segment
{
public:
    block_segment(::std::string_view block_seg_storage)
        : m_storage{ block_seg_storage }
    {
        if ((m_parse_result = parse()) == parse_result_t::error) 
            throw koios::exception{"block_segment, parse fail"};
    }

    /*! \brief  determine the parse situation.
     *  \attention The content retrived by this class could be used only this functiuon return `parse_result_t::success`
     */
    parse_result_t parse_result() const noexcept { return m_parse_result; }

    ::std::string_view public_prefix() const noexcept { return m_prefix; }
    size_t count() const noexcept { return m_items.size(); }
    const auto& items() const noexcept { return m_items; }

private:
    parse_result_t parse();

private:
    ::std::string_view m_storage;
    ::std::string_view m_prefix;
    ::std::vector<::std::string_view> m_items;
    parse_result_t m_parse_result;
};

class block
{
public:
    block(::std::string_view block_storage);
    
private:
    parse_result_t parse();

private:
    ::std::string_view m_storage;
    ::std::vector<block_segment> m_segments;
};

} // namespace frenzykv

#endif
