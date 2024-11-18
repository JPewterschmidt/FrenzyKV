// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <ranges>

#include "toolpex/assert.h"

#include "frenzykv/env.h"

#include "frenzykv/db/memtable_flusher.h"
#include "frenzykv/db/version_descriptor.h"

#include "frenzykv/util/file_center.h"

#include "frenzykv/persistent/compaction.h"

#include "frenzykv/table/sstable_builder.h"

namespace r = ::std::ranges;
namespace rv = r::views;

namespace frenzykv
{

memtable_flusher::
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

koios::task<> memtable_flusher::
flush_to_disk(::std::unique_ptr<memtable> table)
{
    if (co_await table->empty()) co_return;

    auto sst_guard = co_await m_file_center->get_file(name_a_sst(0));
    auto env = m_deps->env();
    auto file = env->get_seq_writable(env->sstables_path()/sst_guard);
    sstable_builder builder{ 
        *m_deps, 
        m_deps->opt()->level_file_size[0], 
        m_filter, file.get() 
    };

    version_delta delta;

    auto finish_current_buiding = 
    [](sstable_builder& builder, version_delta& delta, seq_writable* file, file_guard sst_guard) 
        -> koios::task<> 
    { 
        co_await builder.finish();
        //co_await file->flush();
        co_await file->sync();
        delta.add_new_file(::std::move(sst_guard));
    };

    // Flush those KV into sstable
    auto list = co_await table->get_storage();
    for (const auto& [k, v] : list)
    {
        bool add_result = co_await builder.add(k, v);
        if (!add_result)
        {
            co_await finish_current_buiding(builder, delta, file.get(), ::std::move(sst_guard));
            sst_guard = co_await m_file_center->get_file(name_a_sst(0));
            file = co_await sst_guard.open_write();

            builder = { 
                *m_deps, 
                m_deps->opt()->level_file_size[0], 
                m_filter, file.get() 
            };
            [[maybe_unused]] bool add_result = co_await builder.add(k, v);
            toolpex_assert(add_result);
        }
    }
    co_await finish_current_buiding(builder, delta, file.get(), sst_guard);
    
    auto mut_new_ver = co_await m_version_center->add_new_version();
    mut_new_ver += delta;
    auto new_ver = mut_new_ver.decay_as_immutable();
    const auto vdname = new_ver.version_desc_name();
    auto ver_file = env->get_seq_writable(env->version_path()/vdname);
    co_await write_version_descriptor(*new_ver, ver_file.get());
    co_await set_current_version_file(*m_deps, vdname);
}

} // namespace frenzykv
