// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "toolpex/assert.h"
#include "frenzykv/table/sstable_getter_from_file_and_cache.h"

namespace frenzykv
{

koios::task<::std::shared_ptr<sstable>> sstable_getter_from_file_and_cache::
get(file_guard fg) const 
{
    auto result = co_await m_cache.find_table_phantom(fg.name());
    if (result) co_return result;
    
    co_return co_await sstable::make(*m_deps, m_filter, co_await fg.open_read());
}

} // namespace frenzykv
