#ifndef FRENZYKV_PERSISTENT_COMPACTION_H
#define FRENZYKV_PERSISTENT_COMPACTION_H

#include "koios/task.h"

#include "frenzykv/io/in_mem_rw.h"
#include "frenzykv/persistent/level.h"
#include "frenzykv/persistent/sstable.h"
#include "frenzykv/persistent/sstable_builder.h"

namespace frenzykv
{

class compactor
{
public:
    compactor(const kvdb_deps& deps, level& l, filter_policy& filter) noexcept
        : m_deps{ &deps }, m_level{ &l }, m_filter_policy{ &filter }
    {
    }

    koios::task<::std::unique_ptr<in_mem_rw>> 
    merge_tables(::std::ranges::range auto& tables, level_t target_level)
    {
        namespace rv = ::std::ranges::views;
        assert(tables.size() >= 2);
        const auto first_two = tables | rv::take(2) | rv::adjacent<2>;
        auto [t1, t2] = *begin(first_two);
        
        auto file = co_await merge_two_tables(t1, t2, target_level);
        for (auto& t : tables | rv::drop(2))
        {
            sstable temp{ *m_deps, m_filter_policy, file.get() };
            file = co_await merge_two_tables(temp, t, target_level);
        }

        assert(file->file_size() > 0);
        co_return file;
    }

private:
    koios::task<::std::unique_ptr<in_mem_rw>>
    merge_two_tables(sstable& lhs, sstable& rhs, level_t l);

private:
    const kvdb_deps* m_deps;
    level* m_level;
    filter_policy* m_filter_policy;
};

} // namespace frenzykv

#endif
