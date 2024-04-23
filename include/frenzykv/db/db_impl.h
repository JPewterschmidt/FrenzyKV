#ifndef FRENZYKV_DB_IMPL_H
#define FRENZYKV_DB_IMPL_H

#include <system_error>
#include <memory>
#include <stop_token>
#include "frenzykv/db/memtable.h"
#include "frenzykv/util/record_writer_wrapper.h"
#include "frenzykv/util/memtable_set.h"
#include "frenzykv/log/logger.h"
#include "koios/coroutine_mutex.h"
#include "frenzykv/db.h"
#include "frenzykv/kvdb_deps.h"
#include "frenzykv/db/kv_entry.h"

namespace frenzykv
{

class db_impl : public db_interface
{
public:
    db_impl(::std::string dbname, const options& opt);
    ~db_impl() noexcept;

    koios::task<size_t> write(write_batch batch) override;

    virtual koios::task<::std::optional<kv_entry>> 
    get(const_bspan key, ::std::error_code& ec_out) noexcept override;

private:
    koios::task<sequenced_key> make_query_key(const_bspan userkey);

private:
    ::std::stop_source m_stp_src;
    ::std::string m_dbname;
    kvdb_deps m_deps;
    logger m_log;
    memtable_set m_memset;
};

} // namespace frenzykv

#endif
