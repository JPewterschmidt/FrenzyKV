#include "frenzykv/stdout_writable.h"
#include "frenzykv/error_category.h"
#include "koios/iouring_awaitables.h"
#include <cstdio>

namespace frenzykv
{

using namespace koios;

koios::exp_taskec<> 
stdout_writable::
append(::std::span<const ::std::byte> buffer) noexcept 
{
    toolpex::unique_posix_fd stdoutfd{::fileno(stdout)};
    if (auto ec = co_await uring::append_all(stdoutfd, buffer); ec)
        co_return koios::unexpected(ec);
    co_return koios::ok();
}

koios::exp_taskec<> 
stdout_writable::
sync() noexcept
{
    co_return koios::ok();
}

koios::exp_taskec<> 
stdout_writable::
flush() noexcept
{
    ::fflush(stdout);
    co_return koios::ok();
}

koios::exp_taskec<>
stdout_writable::
close() noexcept 
{ 
    co_await flush(); 
    co_return koios::ok();
}

} // namespace frenzykv
