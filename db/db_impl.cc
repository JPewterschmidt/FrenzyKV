#include <cassert>
#include <ranges>
#include <list>
#include <vector>
#include <algorithm>
#include <iterator>
#include <filesystem>

#include "toolpex/skip_list.h"

#include "koios/iouring_awaitables.h"
#include "koios/this_task.h"

#include "frenzykv/statistics.h"
#include "frenzykv/error_category.h"
#include "frenzykv/options.h"

#include "frenzykv/util/multi_dest_record_writer.h"
#include "frenzykv/util/stdout_debug_record_writer.h"

#include "frenzykv/log/logger.h"

#include "frenzykv/db/db_impl.h"
#include "frenzykv/db/version_descriptor.h"

#include "frenzykv/persistent/sstable_builder.h"
#include "frenzykv/persistent/compaction.h"

namespace rv = ::std::ranges::views;
namespace r = ::std::ranges;
namespace fs = ::std::filesystem;

namespace frenzykv
{

db_impl::db_impl(::std::string dbname, const options& opt)
    : m_dbname{ ::std::move(dbname) }, 
      m_deps{ opt },
      m_log{ m_deps }, 
      m_filter_policy{ make_bloom_filter(64) }, 
      m_file_center{ m_deps }, 
      m_version_center{ m_file_center },
      m_compactor{ m_deps, m_filter_policy.get() }, 
      m_mem{ ::std::make_unique<memtable>(m_deps) }, 
      m_gcer{ &m_version_center, &m_file_center }, 
      m_flusher{ m_deps, &m_version_center, m_filter_policy.get(), &m_file_center }
{
}

db_impl::~db_impl() noexcept
{
    close().result();
    m_bg_gc_stop_src.request_stop();
}

koios::task<bool> db_impl::init() 
{
    if (m_inited.load())
        co_return true;

    spdlog::debug("db_impl::init() start");
    m_inited.store(true);

    spdlog::debug("db_impl::init() turn_into_scheduler");
    co_await koios::this_task::turn_into_scheduler();

    spdlog::debug("db_impl::init() loading from fs");
    co_await m_file_center.load_files();
    co_await m_version_center.load_current_version();

    sequence_number_t seq_from_seqfile = co_await get_leatest_sequence_number(m_deps);
    m_snapshot_center.set_init_leatest_used_sequence_number(seq_from_seqfile);

    if (co_await m_log.empty())
    {
        co_await m_gcer.do_GC();
        spdlog::debug("db_impl::init() done");
        co_return true;
    }
    
    spdlog::debug("db_impl::init() recoverying from pre-write log");
    auto envp = m_deps.env();
    auto [batch, max_seq_from_log] = co_await recover(envp.get());
    co_await m_log.truncate_file();
    co_await m_mem->insert(::std::move(batch));
    m_snapshot_center.set_init_leatest_used_sequence_number(max_seq_from_log);

    auto lk = co_await m_mem_mutex.acquire();
    auto memp = ::std::exchange(m_mem, ::std::make_unique<memtable>(m_deps));
    lk.unlock();

    spdlog::debug("db_impl::init() recoverying from pre-write log compact and flush and gc");
    co_await m_flusher.flush_to_disk(::std::move(memp));
    co_await may_compact();
    co_await m_gcer.do_GC();

    spdlog::debug("db_impl::init() done");
    co_return true;
}

koios::task<::std::pair<bool, version_guard>> 
db_impl::need_compaction(level_t l)
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
        !m_deps.opt()->is_appropriate_level_file_number(l, num), 
        ::std::move(cur_ver)
    };
}

koios::eager_task<> db_impl::may_compact()
{
    const level_t max_level = m_deps.opt()->max_level;
    auto env = m_deps.env();

    spdlog::debug("start compacting");
    for (level_t l{}; l <= max_level; ++l)
    {
        auto [need, ver] = co_await need_compaction(l);
        if (!need) continue;
        
        // Do the actual compaction
        auto [fake_file, delta] = co_await m_compactor.compact(::std::move(ver), l);

        if (fake_file && fake_file->file_size())
        {
            auto file = co_await m_file_center.get_file(name_a_sst(l + 1));
            auto fp = co_await file.open_write(env.get());
            co_await fake_file->dump_to(*fp);
            co_await fp->sync();
            delta.add_new_file(::std::move(file));
        }

        // Add a new version
        auto cur_v = co_await m_version_center.add_new_version();
        cur_v += delta;

        // Write new version to version descriptor
        const auto new_desc_name = cur_v.version_desc_name();
        auto new_desc = env->get_seq_writable(version_path()/new_desc_name);
        [[maybe_unused]] bool write_ret = co_await write_version_descriptor(*cur_v, new_desc.get());
        assert(write_ret);

        // Set current version
        co_await set_current_version_file(m_deps, new_desc_name);
    }
    spdlog::debug("compacting complete");
}

koios::task<> db_impl::close()
{
    if (!m_inited.load())
        co_return;
    
    m_inited = false;
    auto lk = co_await m_mem_mutex.acquire();
    if (co_await m_mem->empty()) co_return;

    co_await m_flusher.flush_to_disk(::std::move(m_mem));
    co_await may_compact();
    [[maybe_unused]] bool write_ret = co_await write_leatest_sequence_number(
        m_deps, 
        m_snapshot_center.leatest_used_sequence_number()
    );
    assert(write_ret);
    co_await delete_all_prewrite_log();
    co_await m_gcer.do_GC();
    
    co_return;
}

koios::task<> db_impl::delete_all_prewrite_log()
{
    for (const auto& dir_entry : fs::directory_iterator(prewrite_log_path()))
    {
        co_await koios::uring::unlink(dir_entry);
    }
}

koios::task<::std::error_code> 
db_impl::
insert(write_batch batch, write_options opt)
{
    co_await init();

    sequence_number_t seq = m_snapshot_center.get_next_unused_sequence_number(batch.count());
    batch.set_first_sequence_num(seq);

    co_await m_log.insert(batch);
    co_await m_log.may_flush(opt.sync_write);
    
    auto lk = co_await m_mem_mutex.acquire();
    if (! co_await m_mem->could_fit_in(batch))
    {
        auto flushing_file = ::std::move(m_mem);
        m_mem = ::std::make_unique<memtable>(m_deps);
        co_await m_flusher.flush_to_disk(::std::move(flushing_file));
        co_await may_compact();
        do_GC().run();
    }
    co_return co_await m_mem->insert(::std::move(batch));
}

koios::task<> 
db_impl::do_GC()
{
    co_await m_gcer.do_GC();
}

koios::task<::std::optional<kv_entry>> 
db_impl::get(const_bspan key, ::std::error_code& ec_out, read_options opt) noexcept
{
    co_await init();
    snapshot snap = opt.snap.valid() ? ::std::move(opt.snap) : co_await get_snapshot();

    const sequenced_key skey = co_await this->make_query_key(key, snap);
    auto result_opt = co_await m_mem->get(skey);
    if (result_opt) 
    {
        if (!result_opt->is_tomb_stone()) co_return result_opt;
        co_return {};
    }

    co_return co_await find_from_ssts(skey, ::std::move(snap));
}

koios::task<::std::optional<kv_entry>> 
db_impl::find_from_ssts(const sequenced_key& key, snapshot snap) const
{
    version_guard ver = snap.valid() ? snap.version() : co_await m_version_center.current_version();
    auto files = ver.files();
    r::sort(files);

    toolpex::skip_list<sequenced_key, kv_user_value> potiential_results(16);

    auto find_from_potiential_results = 
    [&potiential_results, &key] mutable -> koios::task<::std::optional<kv_entry>> { 
        ::std::optional<kv_entry> result;
        auto iter = potiential_results.find_last_less_equal(key);
        if (iter != potiential_results.end() && !iter->second.is_tomb_stone())
        {
            co_return result.emplace(::std::move(iter->first), ::std::move(iter->second));
        }
        co_return result;
    };

    level_t cur_level{};
    auto env = m_deps.env();
    for (auto fg : files)
    {
        if (level_t l = fg.level(); l != cur_level) 
        {
            cur_level = l;
            if (auto ret = co_await find_from_potiential_results(); ret.has_value())
                co_return ret;
            potiential_results.clear();
        }
        auto filep = co_await fg.open_read(env.get());
        if (filep->file_size() == 0) continue;
        sstable sst(m_deps, m_filter_policy.get(), ::std::move(filep));

        co_await sst.parse_meta_data();
        auto entry_opt = co_await sst.get_kv_entry(key);
        if (!entry_opt.has_value() 
            || (snap.valid() && entry_opt->key().sequence_number() > snap.sequence_number()))
        {
            continue;
        }
        potiential_results.insert(
            ::std::move(entry_opt->key()), 
            ::std::move(entry_opt->value())
        );
    }

    co_return co_await find_from_potiential_results();
}

koios::task<sequenced_key> 
db_impl::make_query_key(const_bspan userkey, const snapshot& snap)
{
    co_return { 
        (snap.valid() 
            ? snap.sequence_number() 
            : m_snapshot_center.leatest_used_sequence_number()), 
        userkey
    };
}

koios::task<snapshot> db_impl::get_snapshot()
{
    co_await init();
    co_return m_snapshot_center.get_snapshot(co_await m_version_center.current_version());
}

} // namespace frenzykv
