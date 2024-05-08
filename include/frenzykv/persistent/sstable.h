#ifndef FRENZYKV_PERSISTENT_SSTABLE_H
#define FRENZYKV_PERSISTENT_SSTABLE_H

#include <optional>
#include <vector>
#include <utility>
#include <list>

#include "koios/task.h"

#include "frenzykv/io/readable.h"
#include "frenzykv/kvdb_deps.h"
#include "frenzykv/db/filter.h"
#include "frenzykv/types.h"
#include "frenzykv/persistent/block.h"
#include "frenzykv/util/compressor.h"
#include "frenzykv/io/inner_buffer.h"
#include "frenzykv/persistent/sstable_builder.h"

namespace frenzykv
{

struct block_with_storage
{
    block_with_storage(block bb, buffer<> sto) noexcept
        : b{ ::std::move(bb) }, s{ ::std::move(sto) }
    {
    }

    block_with_storage(block bb) noexcept
        : b{ ::std::move(bb) }
    {
    }

    block b;

    // Iff s.empty(), then the block uses sstable::s_storage
    buffer<> s;
};

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
            filter_policy* filter, 
            random_readable* file);

    sstable(const kvdb_deps& deps, 
            filter_policy* filter, 
            ::std::unique_ptr<random_readable> file);

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
     *  \param  user_key_ignore_seq Only take the serialized user key part, ignore the seq part
     */
    koios::task<::std::optional<::std::pair<block_segment, block_with_storage>>> 
    get_segment(const sequenced_key& user_key_ignore_seq);

    koios::task<::std::optional<kv_entry>>
    get_kv_entry(const sequenced_key& user_key);

    sequenced_key last_user_key_without_seq() const noexcept;
    sequenced_key first_user_key_without_seq() const noexcept;

    bool overlapped(const sstable& other) const noexcept;
    bool disjoint(const sstable& other) const noexcept;

    bool operator<(const sstable& other) const noexcept
    {
        return first_user_key_without_seq() < other.first_user_key_without_seq();
    }

    // Required by `get_segment()`
    koios::task<::std::optional<block_with_storage>> 
    get_block(uintmax_t offset, btl_t btl);

    koios::task<::std::optional<block_with_storage>> 
    get_block(::std::pair<uintmax_t, btl_t> p)
    {
        return get_block(p.first, p.second);
    }

    koios::generator<::std::pair<uintmax_t, btl_t>> block_offsets() const noexcept;
    koios::task<bool>   parse_meta_data();

private:
    koios::task<btl_t>  btl_value(uintmax_t offset);        // Required by `generate_block_offsets()`
    koios::task<bool>   generate_block_offsets(mbo_t mbo);  // Required by `parse_meta_data()`
    
private:
    ::std::unique_ptr<random_readable> m_self_managed_file{};
    const kvdb_deps* m_deps{};
    bool m_meta_data_parsed{};
    random_readable* m_file;
    ::std::string m_filter_rep;
    ::std::string m_first_uk;
    ::std::string m_last_uk;
    filter_policy* m_filter;
    ::std::shared_ptr<compressor_policy> m_compressor;
    ::std::vector<::std::pair<uintmax_t, btl_t>> m_block_offsets;
    buffer<> m_buffer{};
    size_t m_get_call_count{};
};

koios::task<::std::list<kv_entry>>
get_entries_from_sstable(sstable& table);

} // namespace frenzykv

#endif
