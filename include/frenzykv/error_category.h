#ifndef FRENZYKV_ERROR_CATEGORY_H
#define FRENZYKV_ERROR_CATEGORY_H

#include <system_error>

namespace frenzykv
{

const ::std::error_category& kvdb_category() noexcept;

enum status_t : int
{
    FRZ_KVDB_OK = 0, 
    FRZ_KVDB_NOTFOUND, 
    FRZ_KVDB_CORRUPTION, 
    FRZ_KVDB_NOT_IMPLEMENTED, 
    FRZ_KVDB_INVALID_ARGUMENT, 
    FRZ_KVDB_IO_ERROR, 
    FRZ_KVDB_UNKNOW,
    FRZ_KVDB_OUT_OF_RANGE,
    FRZ_KVDB_EXCEPTION_CATCHED,
};

::std::error_code make_frzkv_ok() noexcept;
::std::error_code make_frzkv_out_of_range() noexcept;
::std::error_code make_frzkv_exception_catched() noexcept;

} // namespace frenzykv


#endif
