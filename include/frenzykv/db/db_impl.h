#ifndef FRENZYKV_DB_IMPL_H
#define FRENZYKV_DB_IMPL_H

#include <system_error>
#include <memory>
#include <stop_token>
#include <utility>

#include "koios/coroutine_mutex.h"
#include "koios/coroutine_shared_mutex.h"

#include "frenzykv/kvdb_deps.h"
#include "frenzykv/log/logger.h"
#include "frenzykv/io/in_mem_rw.h"
#include "frenzykv/util/record_writer_wrapper.h"
#include "frenzykv/util/file_center.h"

#include "frenzykv/db.h"
#include "frenzykv/db/kv_entry.h"
#include "frenzykv/db/memtable.h"
#include "frenzykv/db/memtable_flusher.h"
#include "frenzykv/db/read_write_options.h"
#include "frenzykv/db/filter.h"
#include "frenzykv/db/version.h"
#include "frenzykv/db/snapshot.h"
#include "frenzykv/db/garbage_collector.h"

#include "frenzykv/persistent/sstable.h"
#include "frenzykv/persistent/compaction.h"

namespace frenzykv
{

class db_impl : public db_interface
{
public:
    db_impl(::std::string dbname, const options& opt);
    ~db_impl() noexcept;

    koios::task<bool> init() override;

    koios::task<::std::error_code> 
    insert(write_batch batch, write_options opt = {}) override;

    koios::task<::std::optional<kv_entry>> 
    get(const_bspan key, ::std::error_code& ec_out, read_options opt = {}) noexcept override;

    koios::task<> close() override;
    koios::task<snapshot> get_snapshot() override;

    koios::eager_task<> compact_tombstones();

private:
    koios::task<sequenced_key> make_query_key(const_bspan userkey, const snapshot& snap);
    koios::task<> merge_tables(::std::vector<sstable>& tables, level_t target_l);

    koios::task<::std::unique_ptr<in_mem_rw>>
    merge_two_table(sstable& lhs, sstable& rhs, level_t l);

    koios::task<> do_GC();

    koios::task<::std::optional<kv_entry>> find_from_ssts(const sequenced_key& key, snapshot snap) const;
    koios::task<> delete_all_prewrite_log();

    koios::task<> fake_file_to_disk(::std::unique_ptr<in_mem_rw> fake, version_delta& delta, level_t l);
    koios::task<> fake_file_to_disk(::std::ranges::range auto fakes, version_delta& delta, level_t l)
    {
        for (auto& f : fakes)
        {
            co_await fake_file_to_disk(::std::move(f), delta, l);
        }
    }

    koios::task<> update_current_version(version_delta delta);

    koios::task<::std::pair<bool, version_guard>> need_compaction(level_t l);
    koios::eager_task<> may_compact();

private:
    ::std::string m_dbname;
    kvdb_deps m_deps;
    logger m_log;

    // other===============================
    ::std::unique_ptr<filter_policy> m_filter_policy;
    ::std::stop_source m_bg_gc_stop_src;
    file_center m_file_center;
    version_center m_version_center;
    snapshot_center m_snapshot_center;
    compactor m_compactor;

    // mamtable===============================
    mutable koios::mutex m_mem_mutex;
    ::std::unique_ptr<memtable> m_mem;
    garbage_collector m_gcer;
    memtable_flusher m_flusher;

    ::std::atomic_bool m_inited{};
};  

} // namespace frenzykv

#endif
