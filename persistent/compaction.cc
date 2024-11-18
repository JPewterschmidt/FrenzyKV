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

compactor::compactor(const kvdb_deps& deps, filter_policy* filter) noexcept
    : m_deps{ &deps }, 
      m_filter_policy{ filter }
{
    for (level_t i{}; i < m_deps->opt()->max_level; ++i)
    {
        m_mutexes.emplace_back(new koios::mutex{toolpex::lazy_string_concater{} + "compactor No." + i});
    }
}

koios::task<::std::unique_ptr<in_mem_rw>> 
compactor::merge_tables(::std::vector<::std::shared_ptr<sstable>> table_ptrs, 
                        level_t tables_level) const
{
    assert(table_ptrs.size() >= 2);

    auto file = co_await merge_two_tables(table_ptrs[0], table_ptrs[1], tables_level + 1);

    for (auto t : table_ptrs | rv::drop(2))
    {
        auto temp = co_await sstable::make(*m_deps, m_filter_policy, file.get());
        file = co_await merge_two_tables(temp, t, tables_level + 1);
    }

    co_return file;
}

koios::task<::std::pair<::std::unique_ptr<in_mem_rw>, version_delta>>
compactor::compact(version_guard version, 
                   level_t from, 
                   ::std::unique_ptr<sstable_getter> table_getter, 
                   double thresh_ratio)
{
    auto lk_opt = co_await m_mutexes[from]->try_acquire();   
    if (!lk_opt) co_return {};

    // for debug
    static ::std::unordered_map<level_t, size_t> count{};
    spdlog::debug("compact() start - level: {}, count: {}", from, count[from]++);

    auto policy = make_default_compaction_policy(*m_deps, m_filter_policy);
    auto file_guards = co_await policy->compacting_files(version, from);
    const size_t dropped_sz = static_cast<size_t>(file_guards.size() * (1.0 - thresh_ratio));
    if (dropped_sz) file_guards.resize(file_guards.size() - dropped_sz);   

    version_delta compacted;
    compacted.add_compacted_files(file_guards);

    ::std::vector<::std::shared_ptr<sstable>> tables;
    for (auto& fg : file_guards)
        tables.emplace_back(co_await table_getter->get(fg));

    auto newfile = co_await merge_tables(::std::move(tables), from);

    spdlog::debug("compact() complete - level: {}", from);
    co_return { ::std::move(newfile), ::std::move(compacted) };
}

koios::task<::std::unique_ptr<in_mem_rw>>
compactor::
merge_two_tables(::std::shared_ptr<sstable> lhs, ::std::shared_ptr<sstable> rhs, level_t new_level) const
{
    ::std::vector<::std::unique_ptr<in_mem_rw>> result;

    ::std::list<kv_entry> merged;
    auto merged_gen = koios::merge(
        get_entries_from_sstable(*lhs), 
        get_entries_from_sstable(*rhs)
    );
    co_await merged_gen.to(::std::front_inserter(merged));

    toolpex_assert(::std::is_sorted(merged.rbegin(), merged.rend()));

    merged.erase(::std::unique(merged.begin(), merged.end(), 
                    [](const auto& lhs, const auto& rhs) { 
                        return lhs.key().user_key() == rhs.key().user_key(); 
                    }), 
                 merged.end());

    if (!merged.empty())
    {
        const uintmax_t newfilesizebound = m_deps->opt()->allowed_level_file_size(new_level);
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
