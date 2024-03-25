#include "frenzykv/iouring_readable.h"
#include "toolpex/errret_thrower.h"
#include "koios/iouring_awaitables.h"
#include "koios/task.h"
#include <fcntl.h>

using namespace koios;

namespace frenzykv
{

iouring_readable::iouring_readable(const ::std::filesystem::path& p, const options& opt)
    : posix_base{ toolpex::errret_thrower{} << ::open(p.c_str(), O_RDONLY | O_CLOEXEC) }, 
      m_opt{ &opt }
{
}

koios::task<size_t>
iouring_readable::read(::std::span<::std::byte> dest)
{
    co_return m_fdctx.has_read((co_await uring::read(m_fd, dest, m_fdctx.cursor())).nbytes_delivered());
}

koios::task<size_t> 
iouring_readable::read(::std::span<::std::byte> dest, size_t offset) const
{
    co_return (co_await uring::read(m_fd, dest, offset)).nbytes_delivered();
}

} // namespace frenzykv
