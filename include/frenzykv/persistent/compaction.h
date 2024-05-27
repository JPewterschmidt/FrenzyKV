#ifndef FRENZYKV_PERSISTENT_COMPACTION_H
#define FRENZYKV_PERSISTENT_COMPACTION_H

#include <utility>
#include <memory>
#include <vector>

#include "koios/task.h"

#include "frenzykv/types.h"
#include "frenzykv/io/in_mem_rw.h"
#include "frenzykv/db/version.h"
#include "frenzykv/util/sstable_getter.h"
#include "frenzykv/persistent/sstable.h"
#include "frenzykv/persistent/sstable_builder.h"

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

    /*! \brief Compact the specified version from version `from`
     *  \return A pair consisting a `in_mem_rw` object and a `version_delta` object
     *          if the `in_mem_rw` is nullptr, means that is an empty sstable.
     *          You have to do the null ptr check.
     *
     *  Non of business of sequence_number, cause the snapshot mechinaism 
     *  keep those files of version alive
     */
    koios::task<::std::pair<::std::unique_ptr<in_mem_rw>, version_delta>>
    compact(version_guard version, level_t from, ::std::unique_ptr<sstable_getter> table_getter) const;

//private: // TODO: rewrite the tests
    /*  \brief  Merge sstables
     *  \param  tables a vector conatins several sstables.
     *  \return A `in_mem_rw` contains mergging result. 
     *          You can later dump the file into a real fisk file.
     *          but this file may be nullptr indicates that
     *          compaction result is a empty table. 
     *          So you have to do the null pointer check.
     */
    koios::task<::std::unique_ptr<in_mem_rw>> 
    merge_tables(::std::ranges::range auto& table_ptrs, level_t l) const
    {
        namespace rv = ::std::ranges::views;
        assert(table_ptrs.size() >= 2);

        spdlog::debug("compactor::merge_tables() start");

        const auto first_two = table_ptrs | rv::take(2) | rv::adjacent<2>;
        auto [t1, t2] = *begin(first_two);
        auto file = co_await merge_two_tables(*t1, *t2, l + 1);
        spdlog::debug("compactor::merge_tables() one two tables merging complete");

        for (auto t : table_ptrs | rv::drop(2))
        {
            sstable temp{ *m_deps, m_filter_policy, file.get() };
            file = co_await merge_two_tables(temp, *t, l + 1);
            spdlog::debug("compactor::merge_tables() one two tables merging complete");
        }

        co_return file;
    }

    koios::task<::std::pair<::std::vector<::std::unique_ptr<in_mem_rw>>, version_delta>>
    compact_tombstones(version_guard vg, level_t l) const;

private:
    koios::task<::std::unique_ptr<in_mem_rw>>
    merge_two_tables(sstable& lhs, sstable& rhs, level_t next_of_from) const;

private:
    mutable koios::mutex m_mutex;
    const kvdb_deps* m_deps;
    filter_policy* m_filter_policy;
};

} // namespace frenzykv

#endif
