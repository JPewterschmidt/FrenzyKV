#include "db/db_impl.h"
#include "util/multi_dest_record_writer.h"
#include "util/stdout_debug_record_writer.h"
#include "log/logger.h"

namespace frenzykv
{

db_impl::db_impl(::std::string dbname, const options& opt)
    : m_dbname{ ::std::move(dbname) }, m_opt{ &opt }, 
      m_log{ *m_opt, "0001-test.frzkvlog" }, 
      m_writers{ 
        multi_dest_record_writer{}
            .new_with(logging_record_writer<logging_level::DEBUG>{m_log})
      }
{
}

koios::task<size_t> 
db_impl::write(write_batch batch) 
{
    co_await m_writers.write(batch);
    co_return batch.count();
}

koios::task<::std::optional<entry_pbrep>> 
db_impl::get(const_bspan key, ::std::error_code& ec_out) noexcept
{
    co_return {};
}

} // namespace frenzykv
