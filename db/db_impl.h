#ifndef FRENZYKV_DB_IMPL_H
#define FRENZYKV_DB_IMPL_H

#include <system_error>
#include <memory>
#include <stop_token>
#include "db/memtable.h"
#include "db/version.h"
#include "util/record_writer_wrapper.h"
#include "util/memtable_set.h"
#include "log/logger.h"
#include "koios/coroutine_mutex.h"
#include "frenzykv/db.h"
#include "frenzykv/kvdb_deps.h"

namespace frenzykv
{

class db_impl : public db_interface
{
public:
    db_impl(::std::string dbname, const options& opt);
    ~db_impl() noexcept;

    koios::task<size_t> write(write_batch batch) override;

    virtual koios::task<::std::optional<entry_pbrep>> 
    get(const_bspan key, ::std::error_code& ec_out) noexcept override;

private:
    koios::task<> may_prepare_space(const write_batch& b);

private:
    ::std::stop_source m_stp_src;
    ::std::string m_dbname;
    kvdb_deps m_deps;
    version_hub m_version;
    statistics m_stat;
    logger m_log;
    memtable_set m_memset;
};

} // namespace frenzykv

#endif
