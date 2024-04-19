#ifndef FRENZYKV_BLOCK_H
#define FRENZYKV_BLOCK_H

#include <string_view>
#include <vector>
#include <cstdint>
#include "toolpex/move_only.h"
#include "koios/generator.h"
#include "koios/exceptions.h"
#include "frenzykv/util/parse_result.h"
#include "frenzykv/util/compressor.h"
#include "frenzykv/types.h"
#include "frenzykv/kvdb_deps.h"
#include "frenzykv/db/kv_entry.h"

namespace frenzykv
{

/*
 *  |-------------------------------------------------------------------|
 *  |                            Block                                  |
 *  |-----|------------------------------------------------|----|-------|
 *  |     |                   Block content                | 1B |  4B   |
 *  |-----|----|----|----|------|------|------|-----|------|----|-------|  
 *  | BTL | BS | BS | BS | .... | SBSO | SBSO | ... | NSBS | WC | CRC32 |
 *  |-----|----|----|----|------|------|------|-----|------|----|-------|
 *  |     |    Data             |  Meta Data               |    |       |
 *  |-----|------------------------------------------------|----|-------|
 *        ^                     ^                   ^      ^    ^
 *        |                     |                   |      |    |
 *        |                     |                   |      |   crc32_beg_ptr(storage)
 *        |                     |                   |    wc_beg_ptr(storage) 
 *        |                     |   nsbs_beg_ptr(storage)  ^ 
 *        |          meta_data_beg_ptr(storage)            |
 *   block_content_beg_ptr(storage)                        |
 *        ^                                                |
 *        |                                                |
 *         ------------------------------------------------
 *                                ^
 *                                |
 *              undecompressed_block_content(storage)
 *
 *
 *  BTL:    4B  uint32_t    Block total length
 *  SBSO:   4B  uint32_t    Special Block Segment Offset
 *  NSBS:   2B  uint16_t    Number of Special Block Segment
 *  WC:     1B              Wether compressed(=1) or not(=0)
 *
 *  CRC32:  4B  uint32_t    The CRC32 result of NON-compressed Block content.
 *                          If the value of WC equals 1, the the value of CRC32 
 *                          covers the integrity of compressed data
 *                          Otherwise (the value WC is 0), CRC32 
 *                          covers the integrity of uncompressed data.
 */

using btl_t     = uint32_t;
using nsbs_t    = uint16_t;
using sbso_t    = uint32_t;
using crc32_t   = uint32_t;
using wc_t      = uint8_t;

bool block_content_was_comprssed(const_bspan storage);
const_bspan undecompressed_block_content(const_bspan storage);
crc32_t block_content_crc32_value(const_bspan storage);

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
    /*! \param  Uncompressed block storage
     *
     *  Wont check the CRC32 checksum, since the value of checksum 
     *  depends on wethere compression happened.
     *  Caller should deal with the check sum.
     *  Caller should decompress the block storage.
     */
    block(const_bspan block_storage)
        : m_storage{ block_storage }
    {
        if ((m_parse_result = parse_meta_data()) == parse_result_t::error)
            throw koios::exception{"block_segment: parse fail"};
    }

    const_bspan storage() const noexcept { return m_storage; }
    size_t special_segments_count() const noexcept { return m_special_segs.size(); }

    /*! \brief Get segments from speficied position
     *  \param from A pointer point to a block segment, usually an element of `m_special_segs`.
     */
    koios::generator<block_segment> segments_in_single_interval(::std::vector<const ::std::byte*>::const_iterator from);

    koios::generator<block_segment> segments_in_single_interval()
    {
        return segments_in_single_interval(m_special_segs.begin());
    }

    const auto& special_segment_ptrs() { return m_special_segs; }

private:
    parse_result_t parse_meta_data();

private:
    const_bspan m_storage;
    ::std::vector<const ::std::byte*> m_special_segs;
    parse_result_t m_parse_result{};
};

/*! \brief  The block segment builder
 *
 *  This class will parse kv_entry into a segment, 
 *  all the kv added here must have the same user key as the public_prefix.
 *
 *  This class won't manage storage, 
 *  it just simply append new stuff to a string that you passed as a ctor argument.
 */
class block_segment_builder : public toolpex::move_only
{
public:
    /*! \param dst the storage string
     *  \userkey the userkey which will be the public prefix of this block segment
     */
    block_segment_builder(::std::string& dst, ::std::string_view userkey) noexcept;

    auto public_prefix() const noexcept { return m_public_prefix; }

    /*! \param kv A new kv entry which the user key value and `public_prefix()` are equal.
     *
     *  \retval true Added successfully
     *  \retval false `kv` may have a different user key, or what ever. 
     *                Means that you need to finish this seg builder, 
     *                and replace it with a new one.1
     */
    bool add(const kv_entry& kv);

    /*! \brief Mark the termination of the current segment.
     *  
     *  This will append 4 zero-filled bytes to the storage string.
     */
    void finish();
    bool is_finish() const noexcept { return m_finish; }
    
private:
    ::std::string& m_storage;
    ::std::string_view m_public_prefix;
    bool m_finish{};
};

/*! \brief Convert a range of kv_entry into block.
 *
 *  Assume that the kv_entry range is sorted.
 */
class block_builder : public toolpex::move_only
{
public:
    block_builder(const kvdb_deps& deps, ::std::shared_ptr<compressor_policy> compressor = {});

    void add(const kv_entry& kv);
    ::std::string finish();
    size_t segment_count() const noexcept { return m_seg_count; }
    bool is_finish() const noexcept { return m_finish; }

private:
    ::std::string m_storage;
    const kvdb_deps* m_deps{};
    ::std::unique_ptr<block_segment_builder> m_current_seg_builder;
    size_t m_seg_count{};
    ::std::vector<uint32_t> m_sbsos;
    bool m_finish{};
    ::std::shared_ptr<compressor_policy> m_compressor{};
};

} // namespace frenzykv

#endif