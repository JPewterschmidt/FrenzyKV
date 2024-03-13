#include "db/db_impl.h"
#include "util/multi_dest_record_writer.h"
#include "util/stdout_debug_record_writer.h"

namespace frenzykv
{

db_impl::db_impl(::std::string dbname, const options& opt)
    : m_dbname{ ::std::move(dbname) }, m_opt{ &opt }, 
      m_writers{ 
        multi_dest_record_writer{}
            .new_with(stdout_debug_record_writer{})
            .new_with(stdout_debug_record_writer{})
      }
{
}

koios::task<size_t> 
db_impl::write(write_batch batch) 
{
    co_await m_writers.write(batch);
    co_return {};
}

koios::task<::std::optional<entry_pbrep>> 
db_impl::get(const_bspan key, ::std::error_code& ec_out) noexcept
{
    co_return {};
}

} // namespace frenzykv
