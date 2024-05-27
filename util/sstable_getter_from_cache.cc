#include "frenzykv/util/sstable_getter_from_cache.h"

namespace frenzykv
{

sstable_getter_from_cache::
sstable_getter_from_cache(table_cache& cache) noexcept
    : m_cache{ cache }
{
}

koios::task<::std::shared_ptr<sstable>> sstable_getter_from_cache::
get(const file_guard& fg) const 
{
    return m_cache.insert(fg);   
}

} // namespace frenzykv
