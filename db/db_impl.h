#ifndef FRENZYKV_DB_IMPL_H
#define FRENZYKV_DB_IMPL_H

#include <system_error>
#include <memory>
#include "frenzykv/db.h"
#include "frenzykv/options.h"
#include "db/memtable.h"
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

    koios::task<size_t> write(write_batch batch) override;

    virtual koios::task<::std::optional<entry_pbrep>> 
    get(const_bspan key, ::std::error_code& ec_out) noexcept override;

private:
    ::std::string m_dbname;
    const options* m_opt;   
    logger m_log;
    memtable_set m_memset;
    record_writer_wrapper m_writers;
};

} // namespace frenzykv

#endif
