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
    auto shr = co_await m_update_mutex.acquire_shared();
    auto opt = m_tables.get(name);
    co_return opt ? ::std::move(*opt) : nullptr;
}

koios::task<::std::shared_ptr<sstable>> table_cache::
insert(const file_guard& fg)
{
    auto uni = co_await m_update_mutex.acquire();
    const auto name = fg.name();

    auto mem_file = ::std::make_unique<in_mem_rw>();
    auto env = m_deps->env();
    auto fp = co_await fg.open_read(env.get());
    co_await fp->dump_to(*mem_file);

    auto result = ::std::make_shared<sstable>(*m_deps, m_filter, ::std::move(mem_file));
    m_tables.put(name, result);
    co_return result;   
}
    
} // namespace frenzykv
