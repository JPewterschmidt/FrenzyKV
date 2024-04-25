#include <list>
#include <ranges>
#include <cassert>

#include "frenzykv/persistent/compaction.h"

namespace frenzykv
{

static koios::task<::std::vector<kv_entry>>
get_entries(sstable& table)
{
    ::std::vector<kv_entry> result;
    for (auto blk_off : table.block_offsets())
    {
        auto blk_opt = co_await table.get_block(blk_off);
        assert(blk_opt.has_value());
        for (auto kv : blk_opt->b.entries())
        {
            result.push_back(::std::move(kv));
        }
    }
    assert(::std::is_sorted(result.begin(), result.end()));
    co_return result;
}

koios::task<::std::unique_ptr<in_mem_rw>>
compactor::
merge_two_tables(sstable& lhs, sstable& rhs, level_t l)
{
    const size_t allowed_size = m_level->allowed_file_size(l);
    ::std::vector<::std::unique_ptr<in_mem_rw>> result;

    ::std::vector<kv_entry> lhs_entries = co_await get_entries(lhs);
    ::std::vector<kv_entry> rhs_entries = co_await get_entries(rhs);

    ::std::list<kv_entry> merged;
    ::std::merge(::std::move_iterator{ lhs_entries.begin() }, ::std::move_iterator{ lhs_entries.end() }, 
                 ::std::move_iterator{ rhs_entries.begin() }, ::std::move_iterator{ rhs_entries.end() }, 
                 ::std::back_inserter(merged));

    const auto uniqued_merged = ::std::ranges::unique(merged, 
        [](const auto& lhs, const auto& rhs) { 
            return lhs.key().user_key() == rhs.key().user_key();
        }
    );

    // TODO: remove out-of-date entries
    
    auto file = ::std::make_unique<in_mem_rw>(allowed_size);
    sstable_builder builder{ 
        *m_deps, allowed_size, 
        m_filter_policy, file.get() 
    };
    for (const auto& entry : uniqued_merged)
    {
        if (!co_await builder.add(entry))
        {
            co_await builder.finish();
            result.push_back(::std::move(file));
            file = ::std::make_unique<in_mem_rw>(allowed_size);
            builder = { 
                *m_deps, allowed_size, 
                m_filter_policy, file.get() 
            };
            [[maybe_unused]] bool ret = co_await builder.add(entry);
            assert(ret);
        }
    }
    
    co_return file;
}

} // namespace frenzykv
