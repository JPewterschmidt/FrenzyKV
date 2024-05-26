#include "frenzykv/util/table_cache.h"

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

koios::task<::std::shared_ptr<sstable>> 
table_cache::
find_table(const ::std::string& name)
{
    auto lk = co_await m_mutex.acquire();
    auto result = m_tables.get(name);
    co_return result ? ::std::move(*result) : nullptr;
}

koios::task<::std::shared_ptr<sstable>> table_cache::
insert(const file_guard& fg)
{
    const auto name = fg.name();
    auto result = co_await find_table(name);
    if (result) co_return result;

    auto lk = co_await m_mutex.acquire();

    auto mem_file = ::std::make_unique<in_mem_rw>();
    auto env = m_deps->env();
    auto fp = co_await fg.open_read(env.get());
    co_await fp->dump_to(*mem_file);

    result = ::std::make_shared<sstable>(*m_deps, m_filter, ::std::move(mem_file));
    m_tables.put(name, result);
    co_return result;   
}

koios::task<size_t> table_cache::size() const
{
    auto lk = co_await m_mutex.acquire();
    co_return m_tables.size();
}
    
} // namespace frenzykv
