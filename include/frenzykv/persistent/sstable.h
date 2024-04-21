#ifndef FRENZYKV_PERSISTENT_SSTABLE_H
#define FRENZYKV_PERSISTENT_SSTABLE_H

#include <optional>
#include <vector>

#include "koios/task.h"

#include "frenzykv/io/readable.h"
#include "frenzykv/kvdb_deps.h"
#include "frenzykv/db/filter.h"
#include "frenzykv/types.h"
#include "frenzykv/persistent/block.h"
#include "frenzykv/util/compressor.h"

namespace frenzykv
{

/*
 *      |-------------------|
 *      | Data Block        |
 *      |-------------------|
 *      | Data Block        |
 *      |-------------------|
 *      | Data Block        |
 *      |-------------------|
 *      |         ...       |
 *      |-------------------|
 *      | Meta Block        |
 *      |-------------------|
 *      |  MBO              |   uint64_t    8B
 *      |-------------------|
 *      |  Magic Number     |   uint32_t    4B 
 *      |-------------------|
 *      
 *      MBO             Meta Block Offset   the offset position of Meta block
 *      Meta block      meta_builder.add({0, "bloomfilter"}, m_filter_rep);
 *      Magic Number    See the sstable.cc source file.
 */

class sstable
{
public:
    sstable(const kvdb_deps& deps, 
            ::std::unique_ptr<random_readable> file, 
            ::std::unique_ptr<filter_policy> filter,
            ::std::shared_ptr<compressor_policy> compessor);

    /*! \brief Searching specific user key from this memtable
     *  
     *  \return A ::std::optional<block_segment> 
     *          represents a block segment that all elements 
     *          has the same public prefix (user key for data block).
     *
     *  \attention  `block_segment` only borrow bytes contents 
     *              from some RAII object that actually hold the memory.
     *              So after a call of this function, 
     *              any access to the previously returned 
     *              `block_segment` object are XXX undefined behaviour.
     */
    koios::task<::std::optional<block_segment>> 
    get(const_bspan user_key);

    /*! \brief Searching specific user key from this memtable
     *  
     *  \return A ::std::optional<block_segment> 
     *          represents a block segment that all elements 
     *          has the same public prefix (user key for data block).
     *
     *  \attention  `block_segment` only borrow bytes contents 
     *              from some RAII object that actually hold the memory.
     *              So after a call of this function, 
     *              any access to the previously returned 
     *              `block_segment` object are XXX undefined behaviour.
     */
    koios::task<::std::optional<block_segment>>
    get(::std::string_view user_key) 
    {
        return get(::std::as_bytes(::std::span{user_key}));
    }

private:
    koios::task<bool>   parse_meta_data();
    koios::task<btl_t>  btl_value(uintmax_t offset);    // Required by `generate_block_offsets()`
    koios::task<bool>   generate_block_offsets();       // Required by `parse_meta_data()`

    koios::task<::std::optional<block>> 
    get_block(uintmax_t offset, btl_t btl);             // Required by `get()`
    
private:
    const kvdb_deps* m_deps{};
    bool m_meta_data_parsed{};
    ::std::unique_ptr<random_readable> m_file;
    ::std::string m_filter_rep;
    ::std::unique_ptr<filter_policy> m_filter;
    ::std::shared_ptr<compressor_policy> m_compressor;
    ::std::vector<::std::pair<uintmax_t, btl_t>> m_block_offsets;
    ::std::string m_buffer{};
    size_t m_get_call_count{};
};

} // namespace frenzykv

#endif
