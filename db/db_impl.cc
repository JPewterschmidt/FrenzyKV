#include <cassert>
#include <ranges>
#include <list>
#include <vector>
#include <algorithm>
#include <iterator>

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
      m_level{ m_deps }
{
}

db_impl::~db_impl() noexcept
{
    m_bg_gc_stop_src.request_stop();
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

    co_return {};
}

koios::task<> db_impl::flush_imm_to_sstable()
{
    auto lk = co_await m_mem_mutex.acquire();
    if (m_imm->was_flushed()) co_return;
    
    [[maybe_unused]] auto [id, file] = co_await m_level.create_file(0);
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
            [[maybe_unused]] auto [id, file] = co_await m_level.create_file(0);
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

    may_compact().run();
    
    co_return;
}

koios::eager_task<> 
db_impl::
compact_files(sstable lowlevelt, level_t nextl)
{
    // Select files going to be compacted
    ::std::vector<::std::unique_ptr<random_readable>> files;

    ::std::vector<sstable> tables;
    ::std::vector<file_guard> table_guards;
    for (auto& guard : co_await m_level.level_file_guards(nextl))
    {
        random_readable* filep = files.emplace_back(co_await m_level.open_read(guard)).get();
        sstable potiential_target_table{ m_deps, m_filter_policy.get(), filep };
        if (lowlevelt.overlapped(potiential_target_table))
        {
            tables.emplace_back(::std::move(potiential_target_table));
            table_guards.push_back(guard);
        }
    }

    tables.push_back(::std::move(lowlevelt));
    ::std::sort(tables.begin(), tables.end());

    // Do merging.
    auto mem_file = co_await compactor(
        m_deps, co_await m_level.allowed_file_size(nextl), *m_filter_policy
    ).merge_tables(tables);

    // Allocate a real disk file to flush those data to it.
    auto disk_id_file = co_await m_level.create_file(nextl);
    assert(disk_id_file.second);

    co_await mem_file->dump_to(*disk_id_file.second);
    co_await disk_id_file.second->sync();

    // Record and apply version delta. TODO
    version_delta delta;
    delta.add_compacted_files(table_guards)
         .add_new_file(disk_id_file.first);

    version_guard new_version = co_await m_version_center.add_new_version();
    new_version += delta;

    // write a new version descripter
    ::std::string version_desc_name = get_version_descriptor_name();
    auto version_desc = m_deps.env()->get_seq_writable(version_path()/version_desc_name);
    [[maybe_unused]] bool ret = co_await write_version_descriptor(new_version.rep(), m_level, version_desc.get());
    assert(ret);

    // Set current version
    co_await set_current_version_file(m_deps, version_desc_name);
    co_await m_version_center.set_current_version(new_version);   

    // TODO: Do GC
    
    co_return;
}

koios::task<> db_impl::may_compact()
{
    const size_t level_num = co_await m_level.level_number();
    for (level_t l{}; l < level_num; ++l)
    {
        if (co_await m_level.need_to_comapct(l))
        {
            auto guard = co_await m_level.oldest_file(l);
            auto file = co_await m_level.open_read(guard);
            assert(file);
            co_await compact_files({ m_deps, m_filter_policy.get(), file.get() }, l);
        }
    }
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
    co_await m_version_center.GC();
    co_await m_level.GC();

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

} // namespace frenzykv
