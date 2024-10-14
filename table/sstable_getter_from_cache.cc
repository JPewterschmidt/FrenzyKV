// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "frenzykv/table/sstable_getter_from_cache.h"

namespace frenzykv
{

sstable_getter_from_cache::
sstable_getter_from_cache(table_cache& cache) noexcept
    : m_cache{ cache }
{
}

koios::task<::std::shared_ptr<sstable>> sstable_getter_from_cache::
get(file_guard fg) const 
{
    co_return co_await m_cache.finsert(fg);   
}

} // namespace frenzykv
