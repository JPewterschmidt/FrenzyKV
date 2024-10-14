// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_TABLE_TABLE_CACHE_H
#define FRENZYKV_TABLE_TABLE_CACHE_H

#include <memory>
#include <string>
#include <string_view>

#include "toolpex/lru_cache.h"

#include "koios/coroutine_mutex.h"

#include "fifo_hybrid_lru/fifo_hybrid_lru.h"

#include "frenzykv/kvdb_deps.h"

#include "frenzykv/table/sstable.h"

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

    koios::task<::std::shared_ptr<sstable>> 
    find_table_phantom(const ::std::string& name);

    // Find or insert
    koios::task<::std::shared_ptr<sstable>> finsert(const file_guard& fg, bool phantom = false);
    koios::task<::std::shared_ptr<sstable>> finsert_phantom(const file_guard& fg)
    {
        return finsert(fg, true);
    }

    koios::task<size_t> size() const;
    
private:
    ::std::shared_ptr<sstable>
    find_table_impl(const ::std::string& name);

    ::std::shared_ptr<sstable>
    find_table_phantom_impl(const ::std::string& name);

private:
    const kvdb_deps* m_deps{};
    filter_policy* m_filter{};
    fhl::fifo_hybrid_lru<::std::string, ::std::shared_ptr<sstable>> m_tables;
};

} // namespace frenzykv

#endif
