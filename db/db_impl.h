#ifndef FRENZYKV_DB_IMPL_H
#define FRENZYKV_DB_IMPL_H

#include <system_error>
#include <memory>
#include <stop_token>
#include "frenzykv/db.h"
#include "frenzykv/options.h"
#include "db/memtable.h"
#include "db/version.h"
#include "util/record_writer_wrapper.h"
#include "util/memtable_set.h"
#include "log/logger.h"
#include "koios/coroutine_mutex.h"

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
    ::std::stop_source m_stp_src;
    ::std::string m_dbname;
    options m_opt;   
    ::std::unique_ptr<env> m_env;
    logger m_log;
    memtable_set m_memset;
};

} // namespace frenzykv

#endif
