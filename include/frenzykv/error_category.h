#ifndef FRENZYKV_ERROR_CATEGORY_H
#define FRENZYKV_ERROR_CATEGORY_H

#include <system_error>

namespace frenzykv
{

const ::std::error_category& kvdb_category() noexcept;

enum status_t : int
{
    FRZ_KVDB_OK                = 0, 
    FRZ_KVDB_NOTFOUND          = 1, 
    FRZ_KVDB_CORRUPTION        = 2, 
    FRZ_KVDB_NOT_IMPLEMENTED   = 3, 
    FRZ_KVDB_INVALID_ARGUMENT  = 4, 
    FRZ_KVDB_IO_ERROR          = 5, 
    FRZ_KVDB_UNKNOW            = 6,
};

} // namespace frenzykv


#endif
