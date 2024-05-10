#include <list>
#include <ranges>
#include <cassert>
#include <vector>
#include <ranges>
#include <algorithm>

#include "frenzykv/persistent/compaction.h"
#include "frenzykv/persistent/compaction_policy.h"
#include "frenzykv/persistent/sstable.h"

namespace rv = ::std::ranges::views;

namespace frenzykv
{

koios::task<::std::pair<::std::unique_ptr<in_mem_rw>, version_delta>>
compactor::compact(version_guard version, level_t from)
{
    auto lk = co_await m_mutex.acquire();

    auto policy = make_default_compaction_policy(*m_deps, m_filter_policy);
    auto file_guards = co_await policy->compacting_files(version, from);
    version_delta compacted;
    compacted.add_compacted_files(file_guards);

    auto fileps_view_aw = file_guards
       | rv::transform([this](auto&& fguard) { 
             return fguard.open_read(m_deps->env().get());
         });
    
    ::std::vector files_aw(begin(fileps_view_aw), end(fileps_view_aw));
    ::std::vector<sstable> tables;
    for (auto& filep_aw : files_aw)
        tables.emplace_back(*m_deps, m_filter_policy, co_await filep_aw);

    // To prevent ICE, See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=114850
    auto newfile = co_await merge_tables(tables, from);
    co_return { ::std::move(newfile), ::std::move(compacted) };
}

koios::task<::std::unique_ptr<in_mem_rw>>
compactor::
merge_two_tables(sstable& lhs, sstable& rhs, level_t l)
{
    [[maybe_unused]] bool ok1 = co_await lhs.parse_meta_data();
    [[maybe_unused]] bool ok2 = co_await rhs.parse_meta_data();
    assert(ok1 && ok2);

    ::std::vector<::std::unique_ptr<in_mem_rw>> result;

    ::std::list<kv_entry> lhs_entries; 
    ::std::list<kv_entry> rhs_entries; 

    if (!lhs.empty()) lhs_entries = co_await get_entries_from_sstable(lhs);
    if (!rhs.empty()) rhs_entries = co_await get_entries_from_sstable(rhs);

    ::std::list<kv_entry> merged;
    ::std::merge(::std::move_iterator{ lhs_entries.begin() }, ::std::move_iterator{ lhs_entries.end() }, 
                 ::std::move_iterator{ rhs_entries.begin() }, ::std::move_iterator{ rhs_entries.end() }, 
                 ::std::front_inserter(merged));

    merged.erase(::std::unique(merged.begin(), merged.end(), 
                    [](const auto& lhs, const auto& rhs) { 
                        return lhs.key().user_key() == rhs.key().user_key(); 
                    }), 
                 merged.end());

    ::std::erase_if(merged, is_tomb_stone<kv_entry>);

    if (!merged.empty())
    {
        const uintmax_t newfilesizebound = m_deps->opt()->allowed_level_file_size(l);
        auto file = ::std::make_unique<in_mem_rw>(newfilesizebound);
        sstable_builder builder{ 
            *m_deps, newfilesizebound, 
            m_filter_policy, file.get() 
        };
        for (const auto& entry : merged | rv::reverse)
        {
            [[maybe_unused]] bool add_ret = co_await builder.add(entry);
            assert(add_ret);
        }
        co_await builder.finish();
        co_return file;
    }
    co_return {};
}

} // namespace frenzykv
