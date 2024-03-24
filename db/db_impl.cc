#include "db/db_impl.h"
#include "util/multi_dest_record_writer.h"
#include "util/stdout_debug_record_writer.h"
#include "log/logger.h"
#include "frenzykv/statistics.h"
#include "frenzykv/error_category.h"
#include "frenzykv/options.h"

namespace frenzykv
{

db_impl::db_impl(::std::string dbname, const options& opt)
    : m_dbname{ ::std::move(dbname) }, m_opt{ &opt }, 
      m_log{ *m_opt, "0001-test.frzkvlog" }, 
      m_memset{ *m_opt }
{
}

koios::task<size_t> 
db_impl::write(write_batch batch) 
{
    co_await m_log.write(batch);
    co_await m_memset.insert(::std::move(batch));
    if (co_await m_memset.full())
    {
        co_await flush();
    }
    
    co_return batch.count();
}

koios::task<::std::optional<entry_pbrep>> 
db_impl::get(const_bspan key, ::std::error_code& ec_out) noexcept
{
    co_return {};
}

koios::task<::std::error_code> 
db_impl::flush()
{
   

    co_return {};
}

} // namespace frenzykv
