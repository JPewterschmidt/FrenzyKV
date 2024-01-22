#include "frenzykv/stdout_writable.h"
#include "frenzykv/error_category.h"
#include "koios/iouring_awaitables.h"
#include <cstdio>

namespace frenzykv
{

using namespace koios;

koios::task<size_t> 
stdout_writable::
append(::std::span<const ::std::byte> buffer) 
{
    toolpex::unique_posix_fd stdoutfd{::fileno(stdout)};
    co_await uring::append_all(stdoutfd, buffer);
}

koios::task<> 
stdout_writable::
sync() noexcept
{
    ::fflush(stdout);
    co_return;
}

koios::task<> 
stdout_writable::
flush() noexcept
{
    ::fflush(stdout);
    co_return;
}

koios::task<>
stdout_writable::
close() noexcept 
{ 
    co_await flush(); 
}

} // namespace frenzykv
