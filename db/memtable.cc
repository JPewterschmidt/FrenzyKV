#include "frenzykv/db/memtable.h"
#include "frenzykv/util/entry_extent.h"
#include <utility>
#include <cassert>

namespace frenzykv
{

koios::task<::std::error_code> memtable::insert(const write_batch& b)
{
    auto lk = co_await m_list_mutex.acquire();
    ::std::error_code result{};
    for (const auto& item : b)
    {
        result = insert_impl(item);
        if (result) break;
    }
    co_return result;
}

koios::task<::std::error_code> memtable::insert(write_batch&& b)
{
    auto lk = co_await m_list_mutex.acquire();
    ::std::error_code result{};
    for (auto& item : b)
    {
        result = insert_impl(::std::move(item));
        if (result) break;
    }
    co_return result;
}

::std::error_code memtable::
insert_impl(entry_pbrep&& b)
{
    m_size_bytes += b.ByteSizeLong();
    m_list.insert(::std::move(*b.mutable_key()), ::std::move(*b.mutable_value()));
    return {};
}

::std::error_code memtable::
insert_impl(const entry_pbrep& b)
{
    m_size_bytes += b.ByteSizeLong();
    m_list[b.key()] = b.value();
    return {};
}

static 
::std::optional<entry_pbrep> 
table_get(auto&& list, const auto& key) noexcept
{
    if (auto iter = list.find_bigger_equal(key); 
        iter != list.end())
    {
        entry_pbrep result{};
        construct_regular_entry(&result, iter->first, iter->second);
        return result;
    }

    return {};
}

koios::task<::std::optional<entry_pbrep>> memtable::
get(const seq_key& key) const noexcept
{
    auto lk = co_await m_list_mutex.acquire_shared();
    auto ret = table_get(m_list, key);
    if (ret && is_entry_regular(*ret)) 
        co_return ret;
    co_return {};
}

koios::task<size_t> memtable::count() const
{
    auto lk = co_await m_list_mutex.acquire_shared();
    co_return m_list.size();
}

koios::task<size_t> memtable::bound_size_bytes() const
{
    auto lk = co_await m_list_mutex.acquire_shared();
    assert(m_bound_size_bytes);
    co_return m_bound_size_bytes;
}

koios::task<bool> memtable::full() const
{
    auto lk = co_await m_list_mutex.acquire_shared();
    assert(m_bound_size_bytes);
    co_return m_list.size() == m_bound_size_bytes;
}

koios::task<size_t> memtable::size_bytes() const
{
    auto lk = co_await m_list_mutex.acquire_shared();
    co_return m_size_bytes;
}

koios::task<::std::optional<entry_pbrep>> 
imm_memtable::
get(const seq_key& key) const noexcept
{
    co_return table_get(m_list, key);
}

} // namespace frenzykv
