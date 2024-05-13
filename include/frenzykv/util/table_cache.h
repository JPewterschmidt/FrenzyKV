#ifndef FRENZYKV_UTIL_TABLE_CACHE_H
#define FRENZYKV_UTIL_TABLE_CACHE_H

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
    koios::task<const sstable*> find_table(::std::string_view name);
    koios::task<> insert(sstable table);
    
private:
    toolpex::lru_cache<::std::string, sstable> m_tables;
    mutable koios::shared_mutex m_update_mutex;
};

} // namespace frenzykv

#endif
