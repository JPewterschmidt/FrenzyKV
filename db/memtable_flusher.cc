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

koios::task<::std::pair<bool, version_guard>> 
memtable_flusher::need_compaction(level_t l) const
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

    co_return { 
        !m_deps->opt()->is_appropriate_level_file_number(l, num), 
        ::std::move(cur_ver)
    };
}

koios::eager_task<> memtable_flusher::may_compact(bool joined_gc)
{
    const level_t max_level = m_deps->opt()->max_level;
    bool need_gc{};
    auto env = m_deps->env();
    {
        spdlog::debug("start compacting");
        auto lk = co_await m_compact_mutex.acquire();
        for (level_t l{}; l <= max_level; ++l)
        {
            auto [need, ver] = co_await need_compaction(l);
            if (!need) continue;
            need_gc = true;
            compactor comp{ 
                *m_deps,
                m_deps->opt()->allowed_level_file_size(l), 
                m_filter
            };
            
            // Do the actual compaction
            auto [fake_file, delta] = co_await comp.compact(::std::move(ver), l);

            if (fake_file && fake_file->file_size())
            {
                auto file = co_await m_file_center->get_file(name_a_sst(l + 1));
                auto fp = co_await file.open_write(env.get());
                co_await fake_file->dump_to(*fp);
                co_await fp->sync();
                delta.add_new_file(::std::move(file));
            }

            // Add a new version
            auto cur_v = co_await m_version_center->add_new_version();
            cur_v += delta;

            // Write new version to version descriptor
            const auto new_desc_name = cur_v.version_desc_name();
            auto new_desc = env->get_seq_writable(version_path()/new_desc_name);
            [[maybe_unused]] bool write_ret = co_await write_version_descriptor(*cur_v, new_desc.get());
            assert(write_ret);

            // Set current version
            co_await set_current_version_file(*m_deps, new_desc_name);
        }
        spdlog::debug("compacting complete, iff {}, then issue a gc", need_gc);
    }
    if (need_gc) 
    {
        // TODO For debug
        //joined_gc = true;
        if (joined_gc)
        {
            spdlog::debug("joined gc issued by may_compact start");
            co_await m_gcer->do_GC();
            spdlog::debug("joined gc issued by may_compact completed");
        }
        else 
        {
            spdlog::debug("detach gc issued by may_compact start, no ending message");
            m_gcer->do_GC().run();
        }
    }
}

koios::task<> memtable_flusher::
flush_to_disk(::std::unique_ptr<memtable> table, bool joined_compact)
{
    spdlog::debug("flush_to_disk start");
    {
        // Really important lock, the underlying implmentation guarantee that the FIFO execution order.
        // Which natually form the flushing call into a queue.
        // So this is an easy Consumer/Producer module.
        auto lk = co_await m_mutex.acquire();
        
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
        
        // TODO: refactor the version_center to support disk current setting first, in mem current setting second.
        auto new_ver = co_await m_version_center->add_new_version();
        new_ver += delta;
        const auto vdname = new_ver.version_desc_name();
        auto ver_file = m_deps->env()->get_seq_writable(version_path()/vdname);
        co_await write_version_descriptor(*new_ver, ver_file.get());
        co_await set_current_version_file(*m_deps, vdname);
    }

    // TODO: For debug 
    joined_compact = true;

    // Isssue a potiential compaction
    if (joined_compact) 
    {
        co_await may_compact();
    }
    else 
    {
        may_compact().run();
    }
    spdlog::debug("flush_to_disk end");
}

} // namespace frenzykv
