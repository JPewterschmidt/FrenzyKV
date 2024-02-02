#ifndef FRENZYKV_DB_IMPL_H
#define FRENZYKV_DB_IMPL_H

#include "frenzykv/db.h"

namespace frenzykv
{

class db_impl : public db_interface
{
public:
    koios::task<size_t> write(write_batch batch) override;

    virtual koios::task<::std::optional<entry_pbrep>> 
    get(const_bspan key, ::std::error_code& ec_out) noexcept override;
};

} // namespace frenzykv

#endif
