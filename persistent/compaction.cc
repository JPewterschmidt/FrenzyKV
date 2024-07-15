// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <list>
#include <ranges>
#include <vector>
#include <ranges>
#include <algorithm>

#include "toolpex/assert.h"

#include "frenzykv/persistent/compaction.h"
#include "frenzykv/persistent/compaction_policy.h"
#include "frenzykv/persistent/compaction_policy_tombstone.h"

#include "frenzykv/table/sstable.h"

namespace rv = ::std::ranges::views;

namespace frenzykv
{

koios::task<::std::pair<::std::vector<::std::unique_ptr<in_mem_rw>>, version_delta>>
compactor::compact_tombstones(version_guard vg, level_t l) const
{
    auto lk = co_await m_mutex.acquire();
    spdlog::debug("compact_tombstones() start");
    auto files = co_await compaction_policy_tombstone{*m_deps, m_filter_policy}.compacting_files(vg, l);
    
    ::std::vector<::std::unique_ptr<in_mem_rw>> result;
    version_delta delta;

    auto env = m_deps->env();
    const uintmax_t size_bound = m_deps->opt()->allowed_level_file_size(l);
    for (const file_guard& fg : files)
    {
        auto sst = co_await sstable::make(*m_deps, m_filter_policy, co_await fg.open_read(env.get()));
        if (sst->empty()) [[unlikely]] continue;

        auto entries = co_await get_entries_from_sstable(*sst);
        ::std::erase_if(entries, is_tomb_stone<kv_entry>);
        auto filep = ::std::make_unique<in_mem_rw>();
        sstable_builder builder{ *m_deps, size_bound, m_filter_policy, filep.get() };
        [[maybe_unused]] bool add_ret = co_await builder.add(entries); toolpex_assert(add_ret);       
        [[maybe_unused]] bool finish_ret = co_await builder.finish(); toolpex_assert(finish_ret);

        result.emplace_back(::std::move(filep));
        delta.add_compacted_file(fg);
    }

    spdlog::debug("compact_tombstones() complete");
    co_return { ::std::move(result), ::std::move(delta) };
}

koios::task<::std::pair<::std::unique_ptr<in_mem_rw>, version_delta>>
compactor::compact(version_guard version, level_t from, ::std::unique_ptr<sstable_getter> table_getter) const
{
    auto lk = co_await m_mutex.acquire();
    spdlog::debug("compact() start");

    auto policy = make_default_compaction_policy(*m_deps, m_filter_policy);
    auto file_guards = co_await policy->compacting_files(version, from);
    spdlog::debug("compact() after chosing compacting_files()");

    version_delta compacted;
    compacted.add_compacted_files(file_guards);

    ::std::vector<::std::shared_ptr<sstable>> tables;
    for (auto& fg : file_guards)
        tables.emplace_back(co_await table_getter->get(fg));

    auto newfile = co_await merge_tables(tables, from);

    spdlog::debug("compact() complete");
    co_return { ::std::move(newfile), ::std::move(compacted) };
}

koios::task<::std::unique_ptr<in_mem_rw>>
compactor::
merge_two_tables(sstable& lhs, sstable& rhs, level_t l) const 
{
    ::std::vector<::std::unique_ptr<in_mem_rw>> result;

    ::std::list<kv_entry> lhs_entries; 
    ::std::list<kv_entry> rhs_entries; 

    if (!lhs.empty()) lhs_entries = co_await get_entries_from_sstable(lhs);
    if (!rhs.empty()) rhs_entries = co_await get_entries_from_sstable(rhs);

    ::std::list<kv_entry> merged;
    ::std::merge(::std::move_iterator{ lhs_entries.begin() }, ::std::move_iterator{ lhs_entries.end() }, 
                 ::std::move_iterator{ rhs_entries.begin() }, ::std::move_iterator{ rhs_entries.end() }, 
                 ::std::front_inserter(merged));

    toolpex_assert(::std::is_sorted(merged.rbegin(), merged.rend()));

    merged.erase(::std::unique(merged.begin(), merged.end(), 
                    [](const auto& lhs, const auto& rhs) { 
                        return lhs.key().user_key() == rhs.key().user_key(); 
                    }), 
                 merged.end());

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
            toolpex_assert(add_ret);
        }
        co_await builder.finish();
        co_return file;
    }
    co_return {};
}

} // namespace frenzykv
