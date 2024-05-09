#include "koios/iouring_awaitables.h"

#include "frenzykv/db/garbage_collector.h"

namespace frenzykv
{

koios::eager_task<> garbage_collector::do_GC() const
{
    auto lk = co_await m_mutex.acquire();

    spdlog::debug("gc start");
    auto delete_garbage_version_desc = [](const auto& vrep) -> koios::task<> { 
        assert(vrep.approx_ref_count() == 0);
        co_await koios::uring::unlink(version_path()/vrep.version_desc_name());
    };

    // delete those garbage version descriptor files
    co_await m_version_center->GC_with(delete_garbage_version_desc);
    co_await m_file_center->GC();
    spdlog::debug("gc complete");
}


} // namespace frenzykv
