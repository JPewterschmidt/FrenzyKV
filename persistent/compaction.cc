#include <list>
#include <ranges>
#include <cassert>

#include "frenzykv/persistent/compaction.h"
#include "frenzykv/persistent/sstable.h"

namespace frenzykv
{

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
