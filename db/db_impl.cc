#include "db/db_impl.h"

namespace frenzykv
{

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
