#ifndef FRENZYKV_BLOCK_H
#define FRENZYKV_BLOCK_H

#include <string_view>
#include <vector>
#include <cstdint>
#include "koios/generator.h"
#include "koios/exceptions.h"
#include "frenzykv/util/parse_result.h"
#include "frenzykv/types.h"

namespace frenzykv
{

/*! \brief Segment of a block.
 *
 *  Parsing will be executed during construction. Not lazy evaluation.
 */
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
    const_bspan storage() const noexcept { return m_storage; }

    bool fit_public_prefix(const_bspan user_prefix) const noexcept;

private:
    parse_result_t parse();

private:
    const_bspan m_storage;
    const_bspan m_prefix;
    ::std::vector<const_bspan> m_items;
    parse_result_t m_parse_result;
};

/*! \brief  Block obejct
 *  Lazy evaluation. But it will parse the meta data during construction.
 */
class block
{
public:
    block(const_bspan block_storage)
        : m_storage{ block_storage }
    {
        if ((m_parse_result = parse_meta_data()) == parse_result_t::error)
            throw koios::exception{"block_segment: parse fail"};
    }

    const_bspan storage() const noexcept { return m_storage; }

private:
    parse_result_t parse_meta_data();

    /*! \brief Get segments from speficied position
     *  \param from A pointer point to a block segment, usually an element of `m_special_segs`.
     */
    koios::generator<block_segment> segments(::std::vector<const ::std::byte*>::const_iterator from);

private:
    const_bspan m_storage;
    ::std::vector<const ::std::byte*> m_special_segs;
    parse_result_t m_parse_result{};
};

} // namespace frenzykv

#endif
