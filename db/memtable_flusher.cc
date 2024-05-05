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

koios::task<bool> memtable_flusher::need_compaction(level_t l) const
{
    auto cur_ver = co_await m_version_center->current_version();
    auto level_files_view = cur_ver.files()
        | rv::transform([](auto&& guard){ return retrive_level_and_id_from_sst_name(guard); })
        | rv::filter([](auto&& opt)     { return opt.has_value();   })
        | rv::transform([](auto&& lopt) { return lopt->first;       })
        | rv::filter([l](auto&& level)  { return level == l;        })
        | rv::transform([]([[maybe_unused]] auto&&){ return 1; })
        ;
    const size_t num = r::fold_left(level_files_view, 0, ::std::plus<size_t>{});
    co_return m_deps->opt()->is_appropriate_level_file_number(l, num);
}

koios::task<> memtable_flusher::
flush_to_disk(::std::unique_ptr<memtable> table)
{
    // Really important lock, the underlying implmentation guarantee that the FIFO execution order.
    // Which natually form the flushing call into a queue.
    // So this is an easy Consumer/Producer module.
    auto lk = co_await m_mutex.acquire();
    auto cur_ver = co_await m_version_center->current_version();
    
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
    for (const auto& [k, v] : *table)
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
    
    // Update current version descriptor
    const auto ver_filename = cur_ver.version_desc_name();
    auto ver_file = m_deps->env()->get_seq_writable(version_path()/ver_filename);
    [[maybe_unused]] bool appending_ret = co_await append_version_descriptor(delta.added_files(), ver_file.get());
    assert(appending_ret);

    // Only after changing the version descriptor (on disk), 
    // the in memory update could be performed.
    cur_ver += delta;
}

} // namespace frenzykv
