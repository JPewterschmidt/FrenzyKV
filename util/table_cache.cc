#include "frenzykv/util/table_cache.h"

namespace frenzykv
{

koios::task<const sstable*> table_cache::
find_table(::std::string_view name)
{
    auto shr = co_await m_update_mutex.acquire_shared();
    toolpex::not_implemented();
    co_return {};
}

koios::task<> table_cache::
insert(sstable table)
{
    auto uni = co_await m_update_mutex.acquire();
    toolpex::not_implemented();
    co_return;
}
    
} // namespace frenzykv
