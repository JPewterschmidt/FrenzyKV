#ifndef FRENZYKV_DB_IMPL_H
#define FRENZYKV_DB_IMPL_H

#include <system_error>
#include <memory>
#include <stop_token>

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
#include "frenzykv/db/read_write_options.h"
#include "frenzykv/db/filter.h"
#include "frenzykv/db/version.h"
#include "frenzykv/db/snapshot.h"

#include "frenzykv/persistent/sstable.h"

namespace frenzykv
{

class db_impl : public db_interface
{
public:
    db_impl(::std::string dbname, const options& opt);
    ~db_impl() noexcept;

    koios::task<::std::error_code> 
    insert(write_batch batch, write_options opt = {}) override;

    koios::task<::std::optional<kv_entry>> 
    get(const_bspan key, ::std::error_code& ec_out, read_options opt = {}) noexcept override;

    koios::task<> close() override;
    koios::task<snapshot> get_snapshot() override;

private:
    koios::task<sequenced_key> make_query_key(const_bspan userkey, const read_options& opt);
    koios::task<> flush_imm_to_sstable();
    koios::task<> merge_tables(::std::vector<sstable>& tables, level_t target_l);

    koios::task<::std::unique_ptr<in_mem_rw>>
    merge_two_table(sstable& lhs, sstable& rhs, level_t l);

    koios::task<> back_ground_GC(::std::stop_token tk);
    koios::task<> do_GC();

private:
    ::std::string m_dbname;
    kvdb_deps m_deps;
    logger m_log;

    // mamtable===============================
    mutable koios::shared_mutex m_mem_mutex;
    ::std::unique_ptr<memtable> m_mem;
    ::std::unique_ptr<imm_memtable> m_imm{};

    // other===============================
    ::std::unique_ptr<filter_policy> m_filter_policy;
    ::std::stop_source m_bg_gc_stop_src;
    file_center m_file_center;
    version_center m_version_center;
    snapshot_center m_snapshot_center;

    // GC
    mutable koios::mutex m_gc_mutex;
};  

} // namespace frenzykv

#endif
