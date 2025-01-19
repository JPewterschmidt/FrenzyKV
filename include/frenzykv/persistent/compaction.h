// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_PERSISTENT_COMPACTION_H
#define FRENZYKV_PERSISTENT_COMPACTION_H

#include <utility>
#include <memory>
#include <vector>
#include <atomic>

#include "koios/task.h"

#include "frenzykv/types.h"
#include "frenzykv/io/in_mem_rw.h"
#include "frenzykv/db/version.h"

#include "frenzykv/table/sstable_getter.h"
#include "frenzykv/table/sstable.h"
#include "frenzykv/table/sstable_builder.h"

namespace frenzykv
{

class compactor
{
public:
    compactor(const kvdb_deps& deps, filter_policy* filter) noexcept;

    /*! \brief  Compact the version `from`
     *  \param  version the version going to be compacted
     *          from the level number of the level going to be compacted
     *          table_getter the table getter which could allows the caller to chose where the sstable be retrived.
     *
     *  \return A vector consisting with `random_readable` objects, usually `in_mem_rw`,
     *          and a `version_delta` object 
     *          which only contains the compacted file info, the caller need to reterive newly added file info
     *          from version center and file center then add to the delta object.
     *          if the `in_mem_rw` is nullptr, means that is an empty sstable.
     *          You have to do the null ptr check.
     *
     *  Non of business of sequence_number, because of the snapshot mechinaism 
     *  keep those files of version alive
     */
    koios::task<::std::pair<::std::vector<::std::unique_ptr<random_readable>>, version_delta>>
    compact(version_guard version, level_t from, 
            ::std::unique_ptr<sstable_getter> table_getter, 
            double thresh_ratio = 1);

    /*  \brief  Merge sstables
     *  \param  tables a vector conatins several sstable pointer (not file pointer).
     *          l the level number of the level those tables belongs.
     *
     *  \return A list that conatains all the kv_entry that tables contain with in a descending order.
     */
    koios::task<::std::list<kv_entry>> 
    merge_tables(::std::vector<::std::shared_ptr<sstable>> table_ptrs, level_t tables_level) const;

    /*  \brief  Functiuon which merges two tables.
     *
     *  \param  lhs the first table  going to be merged.
     *          rhs the second table  going to be merged.
     *          l the new level number merged table belonging.
     *
     *  \return A awaitable object result in a list of kv_entry with descending sort.
     *          This list is the result of merging `lhs`, and `rhs`
     */
    koios::task<::std::list<kv_entry>>
    merge_two_tables(::std::shared_ptr<sstable> lhs, ::std::shared_ptr<sstable> rhs, level_t new_level) const;

    /*  \brief  Functiuon which merges two tables (represented by two descending list of kv_entry).
     *
     *  \param  lhs the first table (represented by a descending list) going to be merged.
     *          rhs the second table (represented by a descending list) going to be merged.
     *          l the new level number merged table belonging.
     *
     *  \return A awaitable object result in a list of kv_entry with descending sort.
     *          This list is the result of merging `descending_lhs`, and `descending_rhs`
     */
    koios::task<::std::list<kv_entry>>
    merge_two_tables(::std::list<kv_entry>&& descending_lhs, ::std::list<kv_entry>&& descending_rhs, level_t new_level) const;

    /*  \brief  Function which convert a descending sorted list of kv_entry to a sstable.
     *  
     *  \param  descending_entries a list that contains kv_entries which probably the result of early stage of merging.
     *          Assume that the `descending_entries` list is the return value of `merge_two_tables` function call.
     */
    koios::task<::std::vector<::std::shared_ptr<sstable>>> 
    merged_list_to_sst(::std::list<kv_entry> descending_entries, level_t new_level) const;

private:
    ::std::vector<::std::unique_ptr<::std::atomic_flag>> m_mutexes{};
    const kvdb_deps* m_deps;
    filter_policy* m_filter_policy;
};

} // namespace frenzykv

#endif
