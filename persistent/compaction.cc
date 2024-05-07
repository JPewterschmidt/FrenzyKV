#include <list>
#include <ranges>
#include <cassert>
#include <vector>
#include <ranges>

#include "frenzykv/persistent/compaction.h"
#include "frenzykv/persistent/compaction_policy.h"
#include "frenzykv/persistent/sstable.h"

namespace rv = ::std::ranges::views;

namespace frenzykv
{

koios::task<::std::pair<::std::unique_ptr<in_mem_rw>, version_delta>>
compactor::compact(version_guard version, level_t from)
{
    auto policy = make_default_compaction_policy(*m_deps, m_filter_policy);
    auto file_guards = co_await policy->compacting_files(version, from);
    version_delta compacted;
    compacted.add_compacted_files(file_guards);

    auto fileps_view = file_guards
       | rv::transform([this](auto&& fguard) { 
             return fguard.open_read(m_deps->env().get());
         });
    
    ::std::vector files(begin(fileps_view), end(fileps_view));
    ::std::vector<sstable> tables;
    for (auto& filep : files)
        tables.emplace_back(*m_deps, m_filter_policy, filep.get());

    // To prevent ICE, See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=114850
    auto newfile = co_await merge_tables(tables);
    co_return { ::std::move(newfile), ::std::move(compacted) };
}

koios::task<::std::unique_ptr<in_mem_rw>>
compactor::
merge_two_tables(sstable& lhs, sstable& rhs)
{
    [[maybe_unused]] bool ok1 = co_await lhs.parse_meta_data();
    [[maybe_unused]] bool ok2 = co_await rhs.parse_meta_data();
    assert(ok1 && ok2);

    ::std::vector<::std::unique_ptr<in_mem_rw>> result;

    ::std::vector<kv_entry> lhs_entries = co_await get_entries_from_sstable(lhs);
    ::std::vector<kv_entry> rhs_entries = co_await get_entries_from_sstable(rhs);
    assert(::std::is_sorted(lhs_entries.begin(), lhs_entries.end()));
    assert(::std::is_sorted(rhs_entries.begin(), rhs_entries.end()));

    ::std::list<kv_entry> merged;
    ::std::merge(::std::move_iterator{ lhs_entries.begin() }, ::std::move_iterator{ lhs_entries.end() }, 
                 ::std::move_iterator{ rhs_entries.begin() }, ::std::move_iterator{ rhs_entries.end() }, 
                 ::std::back_inserter(merged));

    merged.erase(::std::unique(merged.begin(), merged.end(), 
                    [](const auto& lhs, const auto& rhs) { 
                        return lhs.key().user_key() == rhs.key().user_key(); 
                    }), 
                 merged.end());

    auto file = ::std::make_unique<in_mem_rw>(m_newfilesizebound);
    sstable_builder builder{ 
        *m_deps, m_newfilesizebound, 
        m_filter_policy, file.get() 
    };
    for (const auto& entry : merged)
    {
        if (!co_await builder.add(entry))
        {
            co_await builder.finish();
            result.push_back(::std::move(file));
            file = ::std::make_unique<in_mem_rw>(m_newfilesizebound);
            builder = { 
                *m_deps, m_newfilesizebound, 
                m_filter_policy, file.get() 
            };
            [[maybe_unused]] bool ret = co_await builder.add(entry);
            assert(ret);
        }
    }
    co_await builder.finish();
    
    co_return file;
}

} // namespace frenzykv
