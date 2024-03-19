#include "db/memtable.h"
#include <utility>

namespace frenzykv
{

koios::task<> memtable::insert(const write_batch& b)
{
    auto lk = co_await m_list_mutex.acquire();
    for (const auto& item : b)
    {
        co_await insert_impl(item);
    }
}

koios::task<> memtable::insert(write_batch&& b)
{
    auto lk = co_await m_list_mutex.acquire();
    for (auto& item : b)
    {
        co_await insert_impl(::std::move(item));
    }
}

koios::task<> memtable::
insert_impl(entry_pbrep&& b)
{
    ::std::string* v = b.mutable_value();
    ::std::string* k = b.mutable_key();
    m_list.insert(::std::move(*k), ::std::move(*v));
    co_return;
}

koios::task<> memtable::
insert_impl(const entry_pbrep& b)
{
    m_list[b.key()] = b.value();
    co_return;
}

koios::task<entry_pbrep> memtable::
get(const ::std::string& key) const noexcept
{
    auto lk = co_await m_list_mutex.acquire();
    if (auto iter = m_list.find(key); iter != m_list.end())
    {
        entry_pbrep result{};
        result.set_key(key);
        result.set_value(iter->second);
        co_return result;
    }
    co_return {};
}

} // namespace frenzykv
