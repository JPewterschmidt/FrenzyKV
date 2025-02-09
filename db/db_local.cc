// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <chrono>
#include <ranges>
#include <list>
#include <vector>
#include <algorithm>
#include <iterator>
#include <filesystem>
#include <memory>
#include <utility>

#include "toolpex/skip_list.h"
#include "toolpex/assert.h"

#include "koios/iouring_awaitables.h"
#include "koios/this_task.h"

#include "frenzykv/statistics.h"
#include "frenzykv/error_category.h"
#include "frenzykv/options.h"

#include "frenzykv/util/multi_dest_record_writer.h"
#include "frenzykv/util/stdout_debug_record_writer.h"

#include "frenzykv/log/write_ahead_logger.h"

#include "frenzykv/db/db_local.h"
#include "frenzykv/db/version_descriptor.h"

#include "frenzykv/table/sstable_builder.h"
#include "frenzykv/table/sstable_getter_from_cache.h"
#include "frenzykv/table/sstable_getter_from_cache_phantom.h"
#include "frenzykv/table/sstable_getter_from_file.h"
#include "frenzykv/table/sstable_getter_from_file_and_cache.h"

#include "frenzykv/persistent/compaction.h"

namespace rv = ::std::ranges::views;
namespace r = ::std::ranges;
namespace fs = ::std::filesystem;

using namespace ::std::chrono_literals;

namespace frenzykv
{

db_local::db_local(::std::string dbname, options opt)
    : m_dbname{ ::std::move(dbname) }, 
      m_deps{ ::std::move(opt) },
      m_log{ m_deps }, 
      m_filter_policy{ make_bloom_filter(64) }, 
      m_file_center{ m_deps }, 
      m_version_center{ m_file_center },
      m_compactor{ m_deps, m_filter_policy.get() }, 
      m_cache{ m_deps, m_filter_policy.get(), 32 },
      m_mem{ ::std::make_unique<memtable>(m_deps) }, 
      m_gcer{ m_deps, &m_version_center, &m_file_center }, 
      m_flusher{ m_deps, &m_version_center, m_filter_policy.get(), &m_file_center }
{
}

db_local::~db_local() noexcept
{
    close().result();
    m_bg_gc_stop_src.request_stop();
}

koios::task<db_local*> db_local::new_db_local(::std::string dbname, options opt)
{
    db_local* result = new db_local(::std::move(dbname), ::std::move(opt));
    co_await result->init();
    co_return result;
}

koios::task<::std::unique_ptr<db_local>> 
db_local::make_unique_db_local(::std::string dbname, options opt)
{
    auto* result = co_await new_db_local(::std::move(dbname), ::std::move(opt));
    co_return ::std::unique_ptr<db_local>{ result };
}

koios::task<bool> db_local::init() 
{
    auto lk = co_await m_db_status_mutex.acquire();
    if (m_inited)
        co_return true;

    m_inited = true;

    m_num_bound_level0 = m_deps.opt()->allowed_level_file_number(0);

    co_await koios::this_task::yield();

    co_await m_file_center.load_files();
    co_await m_version_center.load_current_version();

    sequence_number_t seq_from_seqfile = co_await get_leatest_sequence_number(m_deps);
    m_snapshot_center.set_init_leatest_used_sequence_number(seq_from_seqfile);

    if (co_await m_log.empty())
    {
        background_compacting_GC(m_bg_gc_stop_src.get_token()).run();
        co_return true;
    }
    
    spdlog::debug("db_local::init() recoverying from pre-write log");
    auto envp = m_deps.env();
    auto [batch, max_seq_from_log] = co_await recover(envp.get());
    co_await m_log.truncate_file();

    auto mem_lk = co_await m_mem_mutex.acquire();
    [[maybe_unused]] auto ec = co_await m_mem->insert(::std::move(batch));
    //toolpex_assert(ec.value() == 0);
    m_snapshot_center.set_init_leatest_used_sequence_number(max_seq_from_log);

    auto memp = ::std::exchange(m_mem, ::std::make_unique<memtable>(m_deps));
    mem_lk.unlock();

    spdlog::debug("db_local::init() recoverying from pre-write log compact and flush and gc");
    co_await m_flusher.flush_to_disk(::std::move(memp));
    co_await may_compact();
    m_gcer.do_GC().run();

    background_compacting_GC(m_bg_gc_stop_src.get_token()).run();

    co_return true;
}

koios::task<::std::pair<bool, version_guard>> 
db_local::need_compaction(level_t l, double thresh_ratio)
{
    auto cur_ver = co_await m_version_center.current_version();
    auto level_files_view = cur_ver.files()
        | rv::transform([](auto&& guard){ return retrive_level_and_id_from_sst_name(guard); })
        | rv::filter([](auto&& opt)     { return opt.has_value();   })
        | rv::transform([](auto&& lopt) { return lopt->first;       })
        | rv::filter([l](auto&& level)  { return level == l;        })
        | rv::transform([]([[maybe_unused]] auto&&){ return 1; })
        ;
    const size_t num = r::fold_left(level_files_view, 0, ::std::plus<size_t>{});

    co_return { 
        !m_deps.opt()->is_appropriate_level_file_number(l, num, thresh_ratio),
        ::std::move(cur_ver)
    };
}

koios::lazy_task<> db_local::compact_tombstones()
{
    toolpex::not_implemented();
    co_return;
}

koios::task<> db_local::update_current_version(version_delta delta)
{
    // Add a new version
    auto mut_cur_v = co_await m_version_center.add_new_version();
    mut_cur_v += delta;
    auto cur_v = mut_cur_v.decay_as_immutable();

    // Write new version to version descriptor
    const auto new_desc_name = cur_v.version_desc_name();
    auto env = m_deps.env();
    auto new_desc = env->get_seq_writable(env->version_path()/new_desc_name);
    [[maybe_unused]] bool write_ret = co_await write_version_descriptor(*cur_v, new_desc.get());
    toolpex_assert(write_ret);

    // Set current version
    co_await set_current_version_file(m_deps, new_desc_name);
}

koios::task<> db_local::fake_file_to_disk(::std::unique_ptr<random_readable> fake_file, version_delta& delta, level_t l)
{
    if (fake_file && fake_file->file_size())
    {
        auto file = co_await m_file_center.get_file(name_a_sst(l));
        auto fp = co_await file.open_write();
        co_await fake_file->dump_to(*fp);
        co_await fp->sync();
        delta.add_new_file(::std::move(file));
    }
}

koios::task<> db_local::may_compact(level_t from, double thresh_ratio)
{
    const level_t max_level = m_deps.opt()->max_level;
    auto env = m_deps.env();

    for (level_t l = from; l <= max_level; ++l)
    {
        auto [need, ver] = co_await need_compaction(l, thresh_ratio);
        if (!need) continue;

        if (l >= 2)
        {
            spdlog::info("Compacting files from level {}", l);
        }
        
        // Do the actual compaction
        auto [mem_files, delta] = co_await m_compactor.compact(
            ::std::move(ver), l, 
            ::std::make_unique<sstable_getter_from_file_and_cache>(m_cache, m_deps, m_filter_policy.get()),
            thresh_ratio
        );

        if (!mem_files.empty())
        {
            for (auto& file : mem_files)
                co_await fake_file_to_disk(::std::move(file), delta, l + 1);
            co_await update_current_version(::std::move(delta));
        }
        break;
    }
}

koios::task<> db_local::close()
{
    auto lk = co_await m_db_status_mutex.acquire();
    if (!m_inited)
        co_return;

    m_inited = false;

    // Make sure the background GC stopped.
    m_bg_gc_stop_src.request_stop();
    co_await m_flying_GC_group.wait();

    auto mem_lk = co_await m_mem_mutex.acquire();

    if (co_await m_mem->empty()) co_return;
    co_await m_flusher.flush_to_disk(::std::move(m_mem));
    co_await may_compact();
    [[maybe_unused]] bool write_ret = co_await write_leatest_sequence_number(
        m_deps, 
        m_snapshot_center.leatest_used_sequence_number()
    );
    toolpex_assert(write_ret);
    co_await delete_all_prewrite_log();
    co_await m_gcer.do_GC();
    
    co_return;
}

koios::task<> db_local::delete_all_prewrite_log()
{
    auto env = m_deps.env();
    for (const auto& dir_entry : fs::directory_iterator(env->write_ahead_log_path()))
    {
        co_await koios::uring::unlink(dir_entry);
    }
}

koios::task<::std::error_code> 
db_local::
insert(write_batch batch, write_options opt)
{
    sequence_number_t seq = m_snapshot_center.get_next_unused_sequence_number(batch.count());
    batch.set_first_sequence_num(seq);

    co_await m_log.insert(batch);
    
    auto unilk = co_await m_mem_mutex.acquire();
    ::std::error_code ec{};
    while (is_frzkv_out_of_range(ec = co_await m_mem->insert(batch)))
    {
        auto flushing_file = ::std::move(m_mem);
        m_mem = ::std::make_unique<memtable>(m_deps);
        const size_t gc_hint = m_force_GC_hint.fetch_add(1, ::std::memory_order_acq_rel);
        co_await m_flusher.flush_to_disk(::std::move(flushing_file));
        if (gc_hint % (m_num_bound_level0 - 1) == 0)
        {
            spdlog::debug("force compacting");
            co_await may_compact();
            do_GC().run();
            continue;
        }
    }
    
    co_return ec; 
}

koios::task<> 
db_local::do_GC()
{
    co_await m_gcer.do_GC();
}

koios::task<::std::optional<kv_entry>> 
db_local::get(const_bspan key, ::std::error_code& ec_out, read_options opt) noexcept
{
    snapshot snap = opt.snap.valid() ? ::std::move(opt.snap) : co_await get_snapshot();

    const sequenced_key skey = co_await this->make_query_key(key, snap);

    auto lk = co_await m_mem_mutex.acquire();
    auto result_opt = co_await m_mem->get(skey);
    lk.unlock();

    if (!result_opt) 
    {
        result_opt = co_await find_from_ssts(skey, ::std::move(snap));
    }

    if (result_opt && !result_opt->is_tomb_stone()) co_return result_opt;
    co_return {};
}

static koios::task<::std::optional<kv_entry>>
find_from_potiential_results(auto& potiential_results, const auto& key)
{
    ::std::optional<kv_entry> result;
    auto iter = potiential_results.find_last_less_equal(key);
    if (iter != potiential_results.end())
    {
        co_return result.emplace(::std::move(iter->first), ::std::move(iter->second));
    }
    co_return result;
}

koios::task<::std::optional<::std::pair<sequenced_key, kv_user_value>>> 
db_local::file_to_async_potiential_ret(const file_guard& fg, const sequenced_key& key, const snapshot& snap) const
{
    ::std::shared_ptr<sstable> sst = co_await m_cache.finsert(fg);
    toolpex_assert(sst);

    auto entry_opt = co_await sst->get_kv_entry(key);
    if (!entry_opt.has_value() 
        || (snap.valid() && entry_opt->key().sequence_number() > snap.sequence_number()))
    {
        co_return {};
    }

    co_return ::std::pair{
        ::std::move(entry_opt->key()), 
        ::std::move(entry_opt->value())
    };
}

koios::task<::std::optional<kv_entry>> 
db_local::find_from_ssts(const sequenced_key& key, snapshot snap) const
{
    version_guard ver = snap.valid() ? snap.version() : co_await m_version_center.current_version();
    auto files = ver.files();
    r::sort(files);

    auto env = m_deps.env();

    // Find record from each level *concurrently*
    for (auto [index, files_same_level] : files | rv::chunk_by(file_guard::have_same_level) | rv::enumerate)
    {
        toolpex::skip_list<sequenced_key, kv_user_value> potiential_results(16);
        auto futvec = files_same_level 
                    | rv::transform([&](auto&& f){ return file_to_async_potiential_ret(f, key, snap); }) 
                    | rv::transform([](auto task){ return task.run_and_get_future(); })
                    ;

        potiential_results.insert_range(co_await koios::co_await_all(::std::move(futvec)) 
            | rv::filter([](auto&& opt) { return opt.has_value(); })
            | rv::transform([](auto&& opt) { return ::std::move(opt.value()); })
            );

        if (auto ret = co_await find_from_potiential_results(potiential_results, key); ret.has_value())
            co_return ret;
    }

    co_return {};
}

koios::task<sequenced_key> 
db_local::make_query_key(const_bspan userkey, const snapshot& snap)
{
    co_return { 
        (snap.valid() 
            ? snap.sequence_number() 
            : m_snapshot_center.leatest_used_sequence_number()), 
        userkey
    };
}

koios::task<snapshot> db_local::get_snapshot()
{
    co_return m_snapshot_center.get_snapshot(co_await m_version_center.current_version());
}

koios::lazy_task<> db_local::background_compacting_GC(::std::stop_token stp)
{
    koios::wait_group_guard g{ m_flying_GC_group };

    co_await koios::for_each_dispatch_evenly(
        rv::iota(level_t{0}, m_deps.opt()->max_level), 
        [this] (level_t l, ::std::stop_token stp) mutable -> koios::task<> {
            co_await koios::this_task::sleep_for(10ms * l);
            while (!stp.stop_requested())
            {
                co_await koios::this_task::sleep_for(10ms * (l + 1));
                co_await may_compact(l, 0.7);
                do_GC().run();
            }
        }, 
    stp);
}

} // namespace frenzykv
