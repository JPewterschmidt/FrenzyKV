#include <cassert>
#include <ranges>
#include <list>
#include <vector>
#include <algorithm>
#include <iterator>
#include <filesystem>

#include "toolpex/skip_list.h"

#include "koios/iouring_awaitables.h"

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
      m_log{ m_deps, m_deps.env()->get_seq_writable(prewrite_log_dir().path()/"0001-test.frzkvlog") }, 
      m_filter_policy{ make_bloom_filter(64) }, 
      m_file_center{ m_deps }, 
      m_version_center{ m_file_center },
      m_mem{ ::std::make_unique<memtable>(m_deps) }, 
      m_gcer{ &m_version_center, &m_file_center }, 
      m_flusher{ m_deps, &m_version_center, m_filter_policy.get(), &m_file_center, &m_gcer }
{
}

db_impl::~db_impl() noexcept
{
    m_bg_gc_stop_src.request_stop();
}

koios::task<bool> db_impl::init() 
{
    co_await m_version_center.load_current_version();
    // TODO: Check pre-write log
    // TODO: Recorvery
    co_await do_GC();
    

    co_return true;
}

koios::task<> db_impl::close()
{
    auto lk = co_await m_mem_mutex.acquire();
    if (co_await m_mem->empty()) co_return;
    // TODO flush whole memtable into disk

    co_await m_flusher.flush_to_disk(::std::move(m_mem), true);
    co_await delete_all_prewrite_log();
    
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
    sequence_number_t seq = m_snapshot_center.get_next_unused_sequence_number(batch.count());
    batch.set_first_sequence_num(seq);

    co_await m_log.insert(batch);
    co_await m_log.may_flush(opt.sync_write);
    
    // Not doing mem-imm trasformation, only need shared lock
    auto shah = co_await m_mem_mutex.acquire_shared();
    if (! co_await m_mem->could_fit_in(batch))
    {
        // Need do memtable transformation
        shah.unlock();
        auto unih = co_await m_mem_mutex.acquire();
        if (co_await m_mem->could_fit_in(batch))
        {
            auto ec = co_await m_mem->insert(batch);
            co_return ec;
        }

        // We need make sure that user won't searching a K when that key are waiting to be flushed.

        auto flushing_file = ::std::move(m_mem);
        m_mem = ::std::make_unique<memtable>(m_deps);
        unih.unlock();
        auto ec = co_await m_mem->insert(::std::move(batch));

        co_await m_flusher.flush_to_disk(::std::move(flushing_file));

        co_return ec;
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
    const sequenced_key skey = co_await this->make_query_key(key, opt);
    auto result_opt = co_await m_mem->get(skey);
    if (result_opt) co_return result_opt;

    co_return co_await find_from_ssts(key, ::std::move(opt.snap));
}

koios::task<::std::optional<kv_entry>> 
db_impl::find_from_ssts(const sequenced_key& key, snapshot snap) const
{
    const version_guard& ver = snap.version();
    auto files = ver.files();
    r::sort(files);

    struct sst_with_file
    {
        sst_with_file(const kvdb_deps& deps, 
                      filter_policy* filter, 
                      ::std::unique_ptr<random_readable> fp, 
                      level_t ll)
            : file{ ::std::move(fp) }, 
              sst{ deps, filter, file.get() }, 
              l { ll }
        {
        }

        ::std::unique_ptr<random_readable> file;
        sstable sst;
        level_t l;
    };

    auto ssts_view = files 
        | rv::transform([this](auto&& fg) { 
              return ::std::pair{ 
                  fg.open_read(m_deps.env().get()), fg.level() 
              }; 
          })
        | rv::transform([&, this](auto&& fp) { 
              return sst_with_file{ 
                  m_deps, m_filter_policy.get(), ::std::move(fp.first), fp.second 
              }; 
          })
        ;

    toolpex::skip_list<sequenced_key, kv_user_value> potiential_results(16);
    level_t cur_level{};
    for (auto sst_f : ssts_view)
    {
        if (sst_f.l != cur_level)
        {
            cur_level = sst_f.l;
            auto iter = potiential_results.find_last_less_equal(key);
            if (iter != potiential_results.end())
            {
                co_return kv_entry{::std::move(iter->first), ::std::move(iter->second)};
            }
            potiential_results.clear();
        }

        auto& sst = sst_f.sst; 
        co_await sst.parse_meta_data();
        auto entry_opt = co_await sst.get_kv_entry(key);
        if (!entry_opt.has_value() || entry_opt->key().sequence_number() > snap.sequence_number())
            continue;
        potiential_results.insert(
            ::std::move(entry_opt->key()), 
            ::std::move(entry_opt->value())
        );
    }

    co_return {};
}

koios::task<sequenced_key> 
db_impl::make_query_key(const_bspan userkey, const read_options& opt)
{
    const auto& snap = opt.snap;
    co_return { 
        (snap.valid() 
            ? snap.sequence_number() 
            : m_snapshot_center.leatest_used_sequence_number()), 
        userkey
    };
}

koios::task<snapshot> db_impl::get_snapshot()
{
    co_return m_snapshot_center.get_snapshot(co_await m_version_center.current_version());
}

} // namespace frenzykv
