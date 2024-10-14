#include "frenzykv/table/sstable_getter_from_cache_phantom.h"

namespace frenzykv
{

sstable_getter_from_cache_phantom::
sstable_getter_from_cache_phantom(table_cache& cache) noexcept
    : m_cache{ cache }
{
}

koios::task<::std::shared_ptr<sstable>> 
sstable_getter_from_cache_phantom::
get(file_guard fg) const
{
    co_return co_await m_cache.finsert_phantom(fg);
}

} // namespace frenzykv
