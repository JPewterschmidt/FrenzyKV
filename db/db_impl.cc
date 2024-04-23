#include <cassert>
#include <ranges>

#include "frenzykv/statistics.h"
#include "frenzykv/error_category.h"
#include "frenzykv/options.h"

#include "frenzykv/util/multi_dest_record_writer.h"
#include "frenzykv/util/stdout_debug_record_writer.h"

#include "frenzykv/log/logger.h"

#include "frenzykv/db/db_impl.h"

#include "frenzykv/persistent/sstable_builder.h"

namespace rv = ::std::ranges::views;

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
    m_stp_src.request_stop();
}

koios::task<::std::error_code> 
db_impl::
insert(write_options write_opt, write_batch batch) 
{
    co_await m_log.insert(batch);
    co_await m_log.may_flush(write_opt.sync_write);
    
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
        m_filter_policy.get(), ::std::move(file) 
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
                m_filter_policy.get(), ::std::move(file)
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

koios::task<>
db_impl::
merge_tables(const ::std::vector<sstable> tables, level_t target_l)
{
    file_id_t builder_file = m_level.create_file(target_l);
    sstable_builder builder{ 
        m_deps, m_level.allowed_file_size(target_l), 
        m_filter_policy.get(), m_level.open_write(builder_file) 
    };

    while (!builder.reach_the_size_limit())
    {
        for (const auto& table : tables)
        {
            // TODO
        }
    }

    co_return;
}

koios::eager_task<> 
db_impl::
compact_files(sstable lowlevelt, level_t nextl)
{
    auto merging_tables = m_level.level_file_ids(nextl)
        | rv::transform(
            [this, nextl](auto&& id) noexcept { 
                return m_level.open_read(nextl, id); 
            })
        | rv::transform(
            [this](auto&& filep) { 
                assert(filep);
                return sstable{ m_deps, m_filter_policy.get(), ::std::move(filep) }; 
            })
        | rv::filter(
            [&lowlevelt](auto&& tab) { 
                return lowlevelt.overlapped(tab); 
            })
        ;

    [[maybe_unused]] ::std::vector<sstable> tables(begin(merging_tables), end(merging_tables));
    ::std::sort(tables.begin(), tables.end());

    co_return;
}

koios::task<> db_impl::may_compact()
{
    const size_t level_num = m_level.level_number();
    for (level_t l{}; l < level_num; ++l)
    {
        if (m_level.need_to_comapct(l))
        {
            const file_id_t target_file = m_level.oldest_file(l);
            auto file = m_level.open_read(l, target_file);
            assert(file);
            co_await compact_files({ m_deps, m_filter_policy.get(), ::std::move(file) }, l);
        }
    }
    co_return;
}

koios::task<::std::optional<kv_entry>> 
db_impl::get(const_bspan key, ::std::error_code& ec_out) noexcept
{
    const sequenced_key skey = co_await this->make_query_key(key);
    auto result_opt = co_await m_mem->get(skey);
    if (result_opt) co_return result_opt;

    co_return {};
}

koios::task<sequenced_key> db_impl::make_query_key(const_bspan userkey)
{
    toolpex::not_implemented();
    co_return {};
}

} // namespace frenzykv
