// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "spdlog/spdlog.h"

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

koios::task<::std::shared_ptr<sstable>> 
table_cache::
find_table(const ::std::string& name)
{
    co_return find_table_impl(name);
}

koios::task<::std::shared_ptr<sstable>> table_cache::
insert(const file_guard& fg)
{
    spdlog::debug("table_cache::insert {}", fg.name());
    const ::std::string& name = fg.name();

    // Find from lru_cache first
    auto result = find_table_impl(name);
    if (result) 
    {
        spdlog::debug("table_cache::insert got existed {}", fg.name());
        co_return result;
    }

    auto mem_file = ::std::make_unique<in_mem_rw>();
    auto env = m_deps->env();
    auto fp = co_await fg.open_read(env.get());
    co_await fp->dump_to(*mem_file);

    result = co_await sstable::make(*m_deps, m_filter, ::std::move(mem_file));

    spdlog::debug("table_cache::insert new {}", fg.name());
    m_tables.put(name, result);
    spdlog::debug("table_cache::inserted new {}", fg.name());

    co_return result;
}

koios::task<size_t> table_cache::size() const
{
    co_return m_tables.size_approx();
}
    
} // namespace frenzykv
