#include <cassert>
#include <ranges>
#include <list>
#include <vector>
#include <algorithm>
#include <iterator>

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

namespace frenzykv
{

db_impl::db_impl(::std::string dbname, const options& opt)
    : m_dbname{ ::std::move(dbname) }, 
      m_deps{ opt },
      m_log{ m_deps, m_deps.env()->get_seq_writable(prewrite_log_dir().path()/"0001-test.frzkvlog") }, 
      m_mem{ ::std::make_unique<memtable>(m_deps) }, 
      m_filter_policy{ make_bloom_filter(64) }, 
      m_file_center{ m_deps }, 
      m_version_center{ m_file_center }
{
}

db_impl::~db_impl() noexcept
{
    m_bg_gc_stop_src.request_stop();
}

koios::task<> db_impl::close()
{
    auto lk = co_await m_mem_mutex.acquire();
    if (co_await m_mem->empty()) co_return;
    // TODO flush whole memtable into disk
    
    co_return;
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

        m_imm = ::std::make_unique<imm_memtable>(::std::move(*m_mem));
        m_mem = ::std::make_unique<memtable>(m_deps);
        auto ec = co_await m_mem->insert(::std::move(batch));

        unih.unlock();

        // Will hold the lock
        flush_imm_to_sstable().run();

        co_return ec;
    }
    co_return co_await m_mem->insert(::std::move(batch));
}

koios::task<> db_impl::flush_imm_to_sstable()
{
    auto lk = co_await m_mem_mutex.acquire();
    if (m_imm->was_flushed()) co_return;
    
    file_guard guard = co_await m_file_center.get_file(name_a_sst(0));
    auto file = guard.open_write(m_deps.env().get());
    const size_t table_size_bound = 4 * 1024 * 1024;

    sstable_builder builder{ 
        m_deps, table_size_bound, 
        m_filter_policy.get(), file.get()
    };
    
    const auto& list = m_imm->storage();
    for (const auto& item : list)
    {
        auto add_ret = co_await builder.add(item.first, item.second);

        // current sstable full
        if (add_ret == false)
        {
            co_await builder.finish();
            guard = co_await m_file_center.get_file(name_a_sst(0));
            file = guard.open_write(m_deps.env().get());
            builder = { 
                m_deps, table_size_bound, 
                m_filter_policy.get(), file.get()
            };

            [[maybe_unused]] auto add_ret2 = co_await builder.add(item.first, item.second);
            assert(add_ret2);
        }
    }

    m_imm->set_flushed_flags();
    lk.unlock();

    co_return;
}

koios::task<> 
db_impl::back_ground_GC(::std::stop_token tk)
{
    while (tk.stop_requested())
    {
        const auto period = m_deps.opt()->gc_period_sec;
        spdlog::debug("background gc sleepping for {}ms", 
                      ::std::chrono::duration_cast<::std::chrono::milliseconds>(period).count()
                     );
        co_await koios::this_task::sleep_for(period);
        spdlog::debug("background gc wake up.");
        co_await do_GC();
    }
}

koios::task<> 
db_impl::do_GC()
{
    auto delete_garbage_version_desc = [](const auto& vrep) -> koios::task<> { 
        assert(vrep.approx_ref_count() == 0);
        co_await koios::uring::unlink(version_path()/vrep.version_desc_name());
    };

    // delete those garbage version descriptor files
    co_await m_version_center.GC_with(delete_garbage_version_desc);
    co_await m_file_center.GC();

    co_return;
}

koios::task<::std::optional<kv_entry>> 
db_impl::get(const_bspan key, ::std::error_code& ec_out, read_options opt) noexcept
{
    const sequenced_key skey = co_await this->make_query_key(key, opt);
    auto result_opt = co_await m_mem->get(skey);
    if (result_opt) co_return result_opt;

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
