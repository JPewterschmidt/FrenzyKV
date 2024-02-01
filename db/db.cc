#include "frenzykv/db.h"

namespace frenzykv
{

koios::task<size_t> 
db_impl::
write(write_batch batch, write_options opt)
{
    co_return {};
}

} // namespace frenzykv
