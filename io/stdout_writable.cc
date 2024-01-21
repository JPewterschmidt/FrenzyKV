#include "frenzykv/stdout_writable.h"
#include "frenzykv/error_category.h"
#include "koios/iouring_awaitables.h"
#include <cstdio>

namespace frenzykv
{

using namespace koios;

koios::task<::std::error_code> 
stdout_writable::
append(::std::span<const ::std::byte> buffer) noexcept 
{
    toolpex::unique_posix_fd stdoutfd{::fileno(stdout)};
    co_return co_await uring::append_all(stdoutfd, buffer);
}

koios::task<::std::error_code> 
stdout_writable::
sync() noexcept
{
    ::fflush(stdout);
    co_return make_frzkv_ok();
}

koios::task<::std::error_code> 
stdout_writable::
flush() noexcept
{
    ::fflush(stdout);
    co_return make_frzkv_ok();
}

koios::taskec 
stdout_writable::
close() noexcept 
{ 
    co_await flush(); 
    co_return make_frzkv_ok(); 
}

} // namespace frenzykv
