// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_PERSISTENT_COMPACTION_H
#define FRENZYKV_PERSISTENT_COMPACTION_H

#include <utility>
#include <memory>
#include <vector>

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
    compactor(const kvdb_deps& deps, filter_policy* filter) noexcept
        : m_deps{ &deps }, 
          m_filter_policy{ filter }
    {
    }

    /*! \brief  Compact the version `from`
     *  \param  version the version going to be compacted
     *          from the level number of the level going to be compacted
     *          table_getter the table getter which could allows the caller to chose where the sstable be retrived.
     *  \return A pair consisting a `in_mem_rw` object and a `version_delta` object
     *          if the `in_mem_rw` is nullptr, means that is an empty sstable.
     *          You have to do the null ptr check.
     *
     *  Non of business of sequence_number, because of the snapshot mechinaism 
     *  keep those files of version alive
     */
    koios::task<::std::pair<::std::unique_ptr<in_mem_rw>, version_delta>>
    compact(version_guard version, level_t from, ::std::unique_ptr<sstable_getter> table_getter) const;

    /*  \brief  Merge sstables
     *  \param  tables a vector conatins several sstable pointer (not file pointer).
     *          l the level number of the level those tables belongs.
     *  \return A `in_mem_rw` contains mergging result. 
     *          You can later dump the file into a real fisk file.
     *          but this file may be nullptr indicates that
     *          compaction result is a empty table. 
     *          So you have to do the null pointer check.
     */
    koios::task<::std::unique_ptr<in_mem_rw>> 
    merge_tables(::std::ranges::range auto& table_ptrs, level_t tables_level) const
    {
        namespace rv = ::std::ranges::views;
        assert(table_ptrs.size() >= 2);

        spdlog::debug("compactor::merge_tables() start");

        const auto first_two = table_ptrs | rv::take(2) | rv::adjacent<2>;
        auto [t1, t2] = *begin(first_two);
        auto file = co_await merge_two_tables(*t1, *t2, tables_level + 1);
        spdlog::debug("compactor::merge_tables() one two tables merging complete");

        for (auto t : table_ptrs | rv::drop(2))
        {
            auto temp = co_await sstable::make(*m_deps, m_filter_policy, file.get());
            file = co_await merge_two_tables(*temp, *t, tables_level + 1);
            spdlog::debug("compactor::merge_tables() one two tables merging complete");
        }

        co_return file;
    }

    koios::task<::std::pair<::std::vector<::std::unique_ptr<in_mem_rw>>, version_delta>>
    compact_tombstones(version_guard vg, level_t l) const;

private:
    /*  \brief  Functiuon which merges two tables.
     *  \param  lhs the first table going to be merged.
     *          rhs the second table going to be merged.
     *          l the new level number merged table belonging.
     *  \return A awaitable object which result is a 
     *          `in_mem_rw` contains merged result. 
     *          You can later dump the file into a real fisk file.
     *          but this file may be nullptr indicates that
     *          compaction result is a empty table. 
     *          So you have to do the null pointer check.
     */
    koios::task<::std::unique_ptr<in_mem_rw>>
    merge_two_tables(sstable& lhs, sstable& rhs, level_t new_level) const;

private:
    mutable koios::mutex m_mutex;
    const kvdb_deps* m_deps;
    filter_policy* m_filter_policy;
};

} // namespace frenzykv

#endif
