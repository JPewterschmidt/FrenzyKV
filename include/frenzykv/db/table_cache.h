#ifndef FRENZYKV_DB_TABLE_CACHE_H
#define FRENZYKV_DB_TABLE_CACHE_H

#include "toolpex/lru_cache.h"

#include "frenzykv/kvdb_deps.h"
#include "frenzykv/io/in_mem_rw.h"
#include "frenzykv/persistent/sstable.h"

namespace frenzykv
{

class table_cache
{
public:
    table_cache(const kvdb_deps& deps)
        : m_deps{ &deps }
    {
    }
    
private:
    const kvdb_deps* m_deps{};
    toolpex::lru_cache</*TODO*/, sstable> m_cache{256};
};

} // namespace frenzykv

#endif
