// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "koios/iouring_awaitables.h"

#include "frenzykv/db/garbage_collector.h"

namespace frenzykv
{

garbage_collector::
garbage_collector(kvdb_deps& deps, version_center* vc, file_center* fc) noexcept
    : m_deps{ &deps }, m_version_center{ vc }, m_file_center{ fc }
{
}

koios::lazy_task<> garbage_collector::do_GC() const
{
    auto lk_opt = co_await m_mutex.try_acquire();
    if (!lk_opt)
        co_return;

    auto delete_garbage_version_desc = [env = this->m_deps->env()](const auto& vrep) -> koios::task<> { 
        assert(vrep.approx_ref_count() == 0);
        co_await koios::uring::unlink(env->version_path()/vrep.version_desc_name());
    };

    // delete those garbage version descriptor files
    co_await m_version_center->GC_with(delete_garbage_version_desc);
    co_await m_file_center->GC();
}

} // namespace frenzykv
