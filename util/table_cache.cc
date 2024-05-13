#include "frenzykv/util/table_cache.h"

namespace frenzykv
{

table_cache::table_cache(size_t capacity)
    : m_tables{ capacity }
{
}

koios::task<::std::shared_ptr<sstable>> 
table_cache::
find_table(const ::std::string& name)
{
    auto shr = co_await m_update_mutex.acquire_shared();
    auto opt = m_tables.get(name);
    co_return opt ? ::std::move(*opt) : nullptr;
}

koios::task<::std::shared_ptr<sstable>> table_cache::
insert(sstable&& table)
{
    auto uni = co_await m_update_mutex.acquire();
    const auto name = table.filename();
    auto result = ::std::make_shared<sstable>(::std::move(table));
    m_tables.put(::std::string(name), result);
    co_return result;   
}
    
} // namespace frenzykv
