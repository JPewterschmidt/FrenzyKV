#ifndef FRENZYKV_PERSISTENT_COMPACTION_H
#define FRENZYKV_PERSISTENT_COMPACTION_H

#include <utility>

#include "koios/task.h"

#include "frenzykv/types.h"
#include "frenzykv/io/in_mem_rw.h"
#include "frenzykv/db/version.h"
#include "frenzykv/persistent/sstable.h"
#include "frenzykv/persistent/sstable_builder.h"

namespace frenzykv
{

class compactor
{
public:
    compactor(const kvdb_deps& deps, 
              uintmax_t newfilesizebound, 
              filter_policy* filter) noexcept
        : m_deps{ &deps }, 
          m_newfilesizebound{ newfilesizebound }, 
          m_filter_policy{ filter }
    {
    }

    // Non of business of sequence_number, cause the snapshot mechinaism 
    // keep those files of version alive
    koios::task<::std::pair<::std::unique_ptr<in_mem_rw>, version_delta>>
    compact(version_guard version, level_t from);

//private: // TODO: rewrite the tests
    /*  \brief  Merge sstables
     *  \param  tables a vector conatins several sstables.
     *  \return A `in_mem_rw` contains mergging result. 
     *          You can later dump the file into a real fisk file.
     */
    koios::task<::std::unique_ptr<in_mem_rw>> 
    merge_tables(::std::ranges::range auto& tables)
    {
        namespace rv = ::std::ranges::views;
        assert(tables.size() >= 2);
        const auto first_two = tables | rv::take(2) | rv::adjacent<2>;
        auto [t1, t2] = *begin(first_two);
        
        auto file = co_await merge_two_tables(t1, t2);
        for (auto& t : tables | rv::drop(2))
        {
            sstable temp{ *m_deps, m_filter_policy, file.get() };
            file = co_await merge_two_tables(temp, t);
        }

        assert(file->file_size() > 0);
        co_return file;
    }

private:
    koios::task<::std::unique_ptr<in_mem_rw>>
    merge_two_tables(sstable& lhs, sstable& rhs);

private:
    const kvdb_deps* m_deps;
    uintmax_t m_newfilesizebound{};
    filter_policy* m_filter_policy;
};

} // namespace frenzykv

#endif
