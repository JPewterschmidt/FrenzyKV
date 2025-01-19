// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <list>
#include <ranges>
#include <vector>
#include <ranges>
#include <algorithm>
#include <span>

#include "toolpex/assert.h"

#include "frenzykv/persistent/compaction.h"
#include "frenzykv/persistent/compaction_policy.h"
#include "frenzykv/persistent/compaction_policy_tombstone.h"

#include "frenzykv/table/sstable.h"

#include "koios/per_consumer_attr.h"
#include "koios/runtime.h"

namespace rv = ::std::ranges::views;
namespace r = ::std::ranges;

namespace frenzykv
{

compactor::compactor(const kvdb_deps& deps, filter_policy* filter) noexcept
    : m_deps{ &deps }, 
      m_filter_policy{ filter }
{
    for (level_t i{}; i < m_deps->opt()->max_level; ++i)
    {
        m_mutexes.emplace_back(new ::std::atomic_flag{ATOMIC_FLAG_INIT});
    }
}

static koios::task<::std::list<kv_entry>>
devide_and_conquer_merge(
    const compactor& c, 
    const ::std::vector<const koios::per_consumer_attr*>& thr_attrs, 
    ::std::span<::std::shared_ptr<sstable>> table_ptrs, 
    level_t new_level)
{
    static unsigned dispatcher{};
    toolpex_assert(table_ptrs.size() != 0);
    if (table_ptrs.size() == 1)
    {
        co_return co_await get_entries_from_sstable_at_once(*table_ptrs[0]);
    }
    
    if (table_ptrs.size() == 2)
    {
        co_return co_await c.merge_two_tables(table_ptrs[0], table_ptrs[1], new_level);
    }

    const size_t attr_sz = thr_attrs.size();
    auto left_fut = devide_and_conquer_merge(c, thr_attrs, table_ptrs.subspan(0, table_ptrs.size() / 2), new_level)
        .run_and_get_future(*thr_attrs[dispatcher++ % attr_sz]);

    auto right_fut = devide_and_conquer_merge(c, thr_attrs, table_ptrs.subspan(table_ptrs.size() / 2), new_level)
        .run_and_get_future(*thr_attrs[dispatcher++ % attr_sz]);

    co_return co_await c.merge_two_tables(co_await left_fut, co_await right_fut, new_level);
}

koios::task<::std::list<kv_entry>> 
compactor::merge_tables(::std::vector<::std::shared_ptr<sstable>> table_ptrs, 
                        level_t tables_level) const
{
    toolpex_assert(table_ptrs.size() >= 2);
    
    const auto& task_consumer_attrs = koios::get_task_scheduler().consumer_attrs();

    co_return co_await devide_and_conquer_merge(*this, task_consumer_attrs, table_ptrs, tables_level + 1);
}

koios::task<::std::pair<::std::vector<::std::unique_ptr<random_readable>>, version_delta>>
compactor::compact(version_guard version, 
                   level_t from, 
                   ::std::unique_ptr<sstable_getter> table_getter, 
                   double thresh_ratio)
{
    if (m_mutexes[from]->test_and_set(::std::memory_order_acquire))
        co_return {};

    class unlocker
    {
    public: 
        unlocker(::std::atomic_flag* f) noexcept : m_flag{ f } {}
        ~unlocker() noexcept { m_flag->clear(::std::memory_order_release); }

    private:
        ::std::atomic_flag* m_flag{};

    } _(m_mutexes[from].get());

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

    auto newtables = co_await merged_list_to_sst(co_await merge_tables(::std::move(tables), from), from + 1);

    spdlog::debug("compact() complete - level: {}", from);
    co_return { 
        newtables | rv::transform([](auto&& item) { return ::std::move(item->unique_file_ptr()); }) 
                  | r::to<::std::vector>(), 
        ::std::move(compacted) 
    };
}

koios::task<::std::list<kv_entry>>
compactor::
merge_two_tables(::std::list<kv_entry>&& lhs, ::std::list<kv_entry>&& rhs, level_t new_level) const
{
    toolpex_assert(!lhs.empty());
    toolpex_assert(!rhs.empty());

    // Descending sorted list natually let the entries with larger sequence number appear first.
    // which will remain after unification.
    toolpex_assert(::std::is_sorted(lhs.rbegin(), lhs.rend()));
    toolpex_assert(::std::is_sorted(rhs.rbegin(), rhs.rend()));

    ::std::list<kv_entry> result;
    ::std::merge(::std::make_move_iterator(lhs.rbegin()), ::std::make_move_iterator(lhs.rend()), 
                 ::std::make_move_iterator(rhs.rbegin()), ::std::make_move_iterator(rhs.rend()), 
                 ::std::front_inserter(result));
    result.erase(::std::unique(result.begin(), result.end(), 
        [](const auto& lhs, const auto& rhs) { 
            return lhs.key().user_key() == rhs.key().user_key(); 
        }), result.end());

    // Consider the result likely to be merged with another table again, 
    // the result list has also to be descending sorted.
    toolpex_assert(::std::is_sorted(result.rbegin(), result.rend()));

    co_return result;
}

koios::task<::std::list<kv_entry>>
compactor::
merge_two_tables(::std::shared_ptr<sstable> lhs, ::std::shared_ptr<sstable> rhs, level_t new_level) const
{
    auto lhs_list = co_await get_entries_from_sstable_at_once(*lhs);
    auto rhs_list = co_await get_entries_from_sstable_at_once(*rhs);

    co_return co_await merge_two_tables(::std::move(lhs_list), ::std::move(rhs_list), new_level);
}

koios::task<::std::vector<::std::shared_ptr<sstable>>> 
compactor::
merged_list_to_sst(::std::list<kv_entry> merged, level_t new_level) const
{
    toolpex_assert(!merged.empty());

    // Assume that teh merged list was the return value of `merge_two_tables` function call.
    toolpex_assert(::std::is_sorted(merged.rbegin(), merged.rend()));
    ::std::vector<::std::shared_ptr<sstable>> result;

    const uintmax_t newfilesizebound = m_deps->opt()->allowed_level_file_size(new_level);
    auto new_builder_and_file = [this, newfilesizebound] { 
        auto file = ::std::make_unique<in_mem_rw>(newfilesizebound);
        return ::std::pair{ sstable_builder{ 
            *m_deps, newfilesizebound, 
            m_filter_policy, file.get() 
        }, ::std::move(file) };
    };
    auto [builder, file] = new_builder_and_file();

    for (const auto& entry : merged | rv::reverse)
    {
         bool add_ret = co_await builder.add(entry);
         if (!add_ret)
         {
             co_await builder.finish();
             result.push_back(co_await sstable::make(*m_deps, m_filter_policy, ::std::move(file)));
             auto [b, f] = new_builder_and_file();
             builder = ::std::move(b);
             file = ::std::move(f);
             add_ret = co_await builder.add(entry);
             toolpex_assert(add_ret);
         }
    }
    co_await builder.finish();
    result.push_back(co_await sstable::make(*m_deps, m_filter_policy, ::std::move(file)));

    co_return result;
}

} // namespace frenzykv
