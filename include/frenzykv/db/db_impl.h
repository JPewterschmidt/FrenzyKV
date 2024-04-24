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

#include "frenzykv/db.h"
#include "frenzykv/db/kv_entry.h"
#include "frenzykv/db/memtable.h"
#include "frenzykv/db/read_write_options.h"
#include "frenzykv/db/filter.h"

#include "frenzykv/persistent/level.h"
#include "frenzykv/persistent/sstable.h"

namespace frenzykv
{

class db_impl : public db_interface
{
public:
    db_impl(::std::string dbname, const options& opt);
    ~db_impl() noexcept;

    koios::task<::std::error_code> insert(write_options write_opt, write_batch batch) override;

    virtual koios::task<::std::optional<kv_entry>> 
    get(const_bspan key, ::std::error_code& ec_out) noexcept override;

private:
    koios::task<sequenced_key> make_query_key(const_bspan userkey);
    koios::task<> flush_imm_to_sstable();
    koios::task<> may_compact();
    koios::eager_task<> compact_files(sstable lowlevelt, level_t nextl);
    koios::task<> merge_tables(const ::std::vector<sstable>& tables, level_t target_l);
    koios::task<::std::vector<::std::unique_ptr<in_mem_rw>>> 
    merge_two_table(const sstable& lhs, const sstable& rhs, level_t l);

private:
    ::std::stop_source m_stp_src;
    ::std::string m_dbname;
    kvdb_deps m_deps;
    logger m_log;

    // mamtable===============================
    mutable koios::shared_mutex m_mem_mutex;
    ::std::unique_ptr<memtable> m_mem;
    ::std::unique_ptr<imm_memtable> m_imm{};

    // other===============================
    ::std::unique_ptr<filter_policy> m_filter_policy;
    level m_level;
};  

} // namespace frenzykv

#endif
