#include "db/memtable.h"
#include <utility>
#include <cassert>

namespace frenzykv
{

koios::task<::std::error_code> memtable::insert(const write_batch& b)
{
    auto lk = co_await m_list_mutex.acquire();
    for (const auto& item : b)
    {
        co_await insert_impl(item);
    }
    co_return {};
}

koios::task<::std::error_code> memtable::insert(write_batch&& b)
{
    auto lk = co_await m_list_mutex.acquire();
    for (auto& item : b)
    {
        co_await insert_impl(::std::move(item));
    }
    co_return {};
}

koios::task<::std::error_code> memtable::
insert_impl(entry_pbrep&& b)
{
    m_size_bytes += b.ByteSizeLong();
    m_list.insert(b.key(), ::std::move(*b.mutable_value()));
    co_return {};
}

koios::task<::std::error_code> memtable::
insert_impl(const entry_pbrep& b)
{
    m_size_bytes += b.ByteSizeLong();
    m_list[b.key()] = b.value();
    co_return {};
}

koios::task<entry_pbrep> memtable::
get(const seq_key& key) const noexcept
{
    auto lk = co_await m_list_mutex.acquire_shared();
    if (auto iter = m_list.find_bigger_equal(key); iter != m_list.end())
    {
        entry_pbrep result{};
        *result.mutable_key() = key;
        // TODO
        result.set_value(iter->second);
        co_return result;
    }
    co_return {};
}

koios::task<size_t> memtable::count() const
{
    auto lk = co_await m_list_mutex.acquire_shared();
    co_return m_list.size();
}

koios::task<size_t> memtable::bound() const
{
    auto lk = co_await m_list_mutex.acquire_shared();
    assert(m_bound);
    co_return m_bound;
}

koios::task<bool> memtable::full() const
{
    auto lk = co_await m_list_mutex.acquire_shared();
    assert(m_bound);
    co_return m_list.size() == m_bound;
}

koios::task<size_t> memtable::size_bytes() const
{
    auto lk = co_await m_list_mutex.acquire_shared();
    co_return m_size_bytes;
}

koios::task<entry_pbrep> 
imm_memtable::
get(const seq_key& key)
{
    co_return co_await m_mem.get(key);
}

} // namespace frenzykv
