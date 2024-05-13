#ifndef FRENZYKV_UTIL_TABLE_CACHE_H
#define FRENZYKV_UTIL_TABLE_CACHE_H

#include <memory>
#include <string>
#include <string_view>

#include "toolpex/lru_cache.h"

#include "koios/coroutine_shared_mutex.h"

#include "frenzykv/persistent/sstable.h"

namespace frenzykv
{

class table_cache
{
public:
    table_cache(size_t capasity);

    koios::task<::std::shared_ptr<sstable>> 
    find_table(const ::std::string& name);

    koios::task<::std::shared_ptr<sstable>> insert(sstable&& table);
    
private:
    toolpex::lru_cache<::std::string, ::std::shared_ptr<sstable>> m_tables;
    mutable koios::shared_mutex m_update_mutex;
};

} // namespace frenzykv

#endif
