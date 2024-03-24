#include "db/memtable.h"
#include <utility>

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
    ::std::string* v = b.mutable_value();
    ::std::string* k = b.mutable_key()->mutable_user_key();
    m_list.insert(::std::move(*k), ::std::move(*v));
    co_return {};
}

koios::task<::std::error_code> memtable::
insert_impl(const entry_pbrep& b)
{
    m_list[b.key().user_key()] = b.value();
    co_return {};
}

koios::task<entry_pbrep> memtable::
get(const ::std::string& key) const noexcept
{
    auto lk = co_await m_list_mutex.acquire_shared();
    if (auto iter = m_list.find(key); iter != m_list.end())
    {
        entry_pbrep result{};
        result.mutable_key()->set_user_key(key);
        // TODO set the seq number 
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
    co_return m_bound;
}

koios::task<bool> memtable::full() const
{
    auto lk = co_await m_list_mutex.acquire_shared();
    co_return m_list.size() == m_bound;
}

koios::task<entry_pbrep> 
imm_memtable::
get(const ::std::string& key)
{
    co_return co_await m_mem.get(key);
}

} // namespace frenzykv
