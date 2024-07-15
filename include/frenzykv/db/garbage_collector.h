// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_DB_GARBAGE_COLLECTOR_H
#define FRENZYKV_DB_GARBAGE_COLLECTOR_H

#include "koios/task.h"
#include "koios/coroutine_mutex.h"

#include "frenzykv/db/version.h"
#include "frenzykv/util/file_center.h"

namespace frenzykv
{

class garbage_collector
{
public:
    garbage_collector(version_center* vc, file_center* fc) noexcept
        : m_version_center{ vc }, m_file_center{ fc }
    {
    }

    koios::eager_task<> do_GC() const;

private:
    version_center* m_version_center{};
    file_center* m_file_center{};
    mutable koios::mutex m_mutex;
};

} // namespace frenzykv

#endif
