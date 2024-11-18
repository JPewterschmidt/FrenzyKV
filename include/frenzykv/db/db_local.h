// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_DB_LOCAL_H
#define FRENZYKV_DB_LOCAL_H

#include <system_error>
#include <memory>
#include <stop_token>
#include <utility>

#include "koios/coroutine_mutex.h"

#include "frenzykv/kvdb_deps.h"
#include "frenzykv/log/write_ahead_logger.h"
#include "frenzykv/io/in_mem_rw.h"
#include "frenzykv/util/record_writer_wrapper.h"
#include "frenzykv/util/file_center.h"

#include "frenzykv/db.h"
#include "frenzykv/db/kv_entry.h"
#include "frenzykv/db/memtable_flusher.h"
#include "frenzykv/db/read_write_options.h"
#include "frenzykv/db/filter.h"
#include "frenzykv/db/version.h"
#include "frenzykv/db/snapshot.h"
#include "frenzykv/db/garbage_collector.h"

#include "frenzykv/table/sstable.h"
#include "frenzykv/table/table_cache.h"
#include "frenzykv/table/memtable.h"

#include "frenzykv/persistent/compaction.h"

namespace frenzykv
{

class db_local : public db_interface
{
private:
    db_local(::std::string dbname, options opt);
    koios::task<bool> init() override;

public:
    ~db_local() noexcept;

    static koios::task<db_local*> new_db_local(::std::string dbname, options opt);

    static koios::task<::std::unique_ptr<db_local>> 
    make_unique_db_local(::std::string dbname, options opt);

    koios::task<::std::error_code> 
    insert(write_batch batch, write_options opt = {}) override;

    koios::task<::std::optional<kv_entry>> 
    get(const_bspan key, ::std::error_code& ec_out, read_options opt = {}) noexcept override;

    koios::task<> close() override;
    koios::task<snapshot> get_snapshot() override;

    koios::lazy_task<> compact_tombstones();

    auto env() const noexcept { return m_deps.env(); }

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

    koios::task<::std::pair<bool, version_guard>> need_compaction(level_t l, double thresh_ratio = 1);
    koios::lazy_task<> may_compact(level_t from = 0, double thresh_ratio = 1);

    koios::lazy_task<> background_compacting_GC(::std::stop_token tk);

    koios::task<::std::optional<::std::pair<sequenced_key, kv_user_value>>> 
    file_to_async_potiential_ret(const file_guard& fg, const sequenced_key& key, const snapshot& snap) const;

private:
    ::std::string m_dbname;

    kvdb_deps m_deps;
    write_ahead_logger m_log;

    // other===============================
    ::std::unique_ptr<filter_policy> m_filter_policy;
    ::std::stop_source m_bg_gc_stop_src;
    file_center m_file_center;
    version_center m_version_center;
    snapshot_center m_snapshot_center;
    compactor m_compactor;
    mutable table_cache m_cache;

    // mamtable===============================
    mutable koios::mutex m_mem_mutex;
    ::std::unique_ptr<memtable> m_mem;
    garbage_collector m_gcer;
    memtable_flusher m_flusher;

    ::std::atomic_bool m_inited{};

    mutable koios::mutex m_flying_GC_mutex;
    ::std::atomic_size_t m_force_GC_hint;
    size_t m_num_bound_level0{};
};  

} // namespace frenzykv

#endif
