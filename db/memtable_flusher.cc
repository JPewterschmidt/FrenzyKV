#include <cassert>
#include <ranges>

#include "frenzykv/env.h"

#include "frenzykv/db/memtable_flusher.h"
#include "frenzykv/db/version_descriptor.h"

#include "frenzykv/util/file_center.h"

#include "frenzykv/persistent/compaction.h"
#include "frenzykv/persistent/sstable_builder.h"

namespace r = ::std::ranges;
namespace rv = r::views;

namespace frenzykv
{

koios::task<> memtable_flusher::
flush_to_disk(::std::unique_ptr<memtable> table)
{
    // Really important lock, the underlying implmentation guarantee that the FIFO execution order.
    // Which natually form the flushing call into a queue.
    // So this is an easy Consumer/Producer module.
    auto lk = co_await m_mutex.acquire();
    
    spdlog::debug("flush_to_disk() start");
    auto sst_guard = co_await m_file_center->get_file(name_a_sst(0));
    auto file = m_deps->env()->get_seq_writable(sstables_path()/sst_guard);
    sstable_builder builder{ 
        *m_deps, 
        m_deps->opt()->level_file_size[0], 
        m_filter, file.get() 
    };

    version_delta delta;

    auto finish_current_buiding = [&] -> koios::task<> { 
        co_await builder.finish();
        co_await file->flush();
        delta.add_new_file(sst_guard);
    };

    // Flush those KV into sstable
    auto list = co_await table->get_storage();
    for (const auto& [k, v] : list)
    {
        bool add_result = co_await builder.add(k, v);
        if (!add_result)
        {
            co_await finish_current_buiding();
            sst_guard = co_await m_file_center->get_file(name_a_sst(0));
            file = m_deps->env()->get_seq_writable(sstables_path()/sst_guard);

            builder = { 
                *m_deps, 
                m_deps->opt()->level_file_size[0], 
                m_filter, file.get() 
            };
            [[maybe_unused]] bool add_result = co_await builder.add(k, v);
            assert(add_result);
        }
    }
    co_await finish_current_buiding();
    
    spdlog::debug("flush_to_disk() updates version info");
    auto new_ver = co_await m_version_center->add_new_version();
    new_ver += delta;
    const auto vdname = new_ver.version_desc_name();
    auto ver_file = m_deps->env()->get_seq_writable(version_path()/vdname);
    co_await write_version_descriptor(*new_ver, ver_file.get());
    co_await set_current_version_file(*m_deps, vdname);

    spdlog::debug("flush_to_disk() complete");
}

} // namespace frenzykv
