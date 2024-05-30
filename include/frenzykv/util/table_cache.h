#ifndef FRENZYKV_UTIL_TABLE_CACHE_H
#define FRENZYKV_UTIL_TABLE_CACHE_H

#include <memory>
#include <string>
#include <string_view>

#include "toolpex/lru_cache.h"

#include "koios/coroutine_mutex.h"

#include "frenzykv/kvdb_deps.h"

#include "frenzykv/persistent/sstable.h"

#include "frenzykv/db/filter.h"

#include "frenzykv/util/file_guard.h"

namespace frenzykv
{

class table_cache
{
public:
    table_cache(const kvdb_deps& deps, filter_policy* filter, size_t capasity);

    koios::task<::std::shared_ptr<sstable>> 
    find_table(const ::std::string& name);

    koios::task<::std::shared_ptr<sstable>> insert(const file_guard& fg);
    koios::task<size_t> size() const;
    
private:
    ::std::shared_ptr<sstable>
    find_table_impl(const ::std::string& name);

private:
    const kvdb_deps* m_deps{};
    filter_policy* m_filter{};
    toolpex::lru_cache<::std::string, ::std::shared_ptr<sstable>> m_tables;
    mutable koios::mutex m_mutex;
};

} // namespace frenzykv

#endif
