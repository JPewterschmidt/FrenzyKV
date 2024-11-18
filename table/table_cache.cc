// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "frenzykv/table/table_cache.h"

#include "frenzykv/io/in_mem_rw.h"

namespace frenzykv
{

table_cache::table_cache(const kvdb_deps& deps, 
                         filter_policy* filter, 
                         size_t capacity)
    : m_deps{ &deps },
      m_filter{ filter }, 
      m_tables{ capacity }
{
}

::std::shared_ptr<sstable>
table_cache::
find_table_impl(const ::std::string& name)
{
    auto result = m_tables.get(name);
    return result ? ::std::move(*result) : nullptr;
}

::std::shared_ptr<sstable>
table_cache::
find_table_phantom_impl(const ::std::string& name)
{
    auto result = m_tables.phantom_get(name);
    return result ? ::std::move(*result) : nullptr;
}

koios::task<::std::shared_ptr<sstable>> 
table_cache::
find_table(const ::std::string& name)
{
    co_return find_table_impl(name);
}

koios::task<::std::shared_ptr<sstable>> 
table_cache::
find_table_phantom(const ::std::string& name)
{
    co_return find_table_phantom_impl(name);
}

koios::task<::std::shared_ptr<sstable>> table_cache::
finsert(const file_guard& fg, bool phantom)
{
    const ::std::string& name = fg.name();

    // Find from lru_cache first
    auto result = phantom ? find_table_phantom_impl(name) : find_table_impl(name);
    if (result) 
    {
        co_return result;
    }

    auto mem_file = ::std::make_unique<in_mem_rw>();
    auto fp = co_await fg.open_read();
    co_await fp->dump_to(*mem_file);

    result = co_await sstable::make(*m_deps, m_filter, ::std::move(mem_file));

    if (!phantom) 
    {
        m_tables.put(name, result);
    }

    co_return result;
}

koios::task<size_t> table_cache::size() const
{
    co_return m_tables.size_approx();
}
    
} // namespace frenzykv
