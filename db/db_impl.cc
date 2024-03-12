#include "db/db_impl.h"

namespace frenzykv
{

db_impl::db_impl(::std::string dbname, const options& opt)
    : m_dbname{ ::std::move(dbname) }, m_opt{ &opt }
{
}

koios::task<size_t> 
db_impl::write(write_batch batch) 
{
       
    co_return {};
}

koios::task<::std::optional<entry_pbrep>> 
db_impl::get(const_bspan key, ::std::error_code& ec_out) noexcept
{
    co_return {};
}

} // namespace frenzykv
