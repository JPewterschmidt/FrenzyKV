// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_MEMTABLE_FLUSHER_H
#define FRENZYKV_MEMTABLE_FLUSHER_H

#include <memory>
#include <utility>
#include <atomic>

#include "koios/coroutine_mutex.h"

#include "frenzykv/types.h"

#include "frenzykv/kvdb_deps.h"
#include "frenzykv/db/version.h"
#include "frenzykv/table/memtable.h"
#include "frenzykv/db/filter.h"
#include "frenzykv/db/garbage_collector.h"

namespace frenzykv
{

class memtable_flusher
{
public:
    memtable_flusher(const kvdb_deps& deps, 
                     version_center* vc, 
                     filter_policy* filter, 
                     file_center* filec) noexcept
        : m_deps{ &deps }, 
          m_version_center{ vc },
          m_filter{ filter }, 
          m_file_center{ filec }
    {
    }

    // This function usually called with `.run()`
    koios::task<> flush_to_disk(::std::unique_ptr<memtable> table);

private:
    const kvdb_deps* m_deps{};
    version_center* m_version_center{};
    filter_policy* m_filter{};
    file_center* m_file_center{};
    mutable koios::mutex m_mutex;
    koios::mutex m_compact_mutex;
};

} // namespace frenzykv

#endif
