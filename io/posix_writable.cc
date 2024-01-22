#include "frenzykv/posix_writable.h"
#include "frenzykv/error_category.h"
#include "toolpex/errret_thrower.h"
#include "koios/iouring_awaitables.h"
#include <fcntl.h>

namespace frenzykv
{

using namespace koios;

posix_writable::
posix_writable(::std::filesystem::path path, options opt, mode_t create_mode)
    : m_options{ ::std::move(opt) }, 
      m_path{ ::std::move(path) }, 
      m_buffer{ buffer_size_nbytes() }
{
    const int open_flags = O_CREAT 
                         | O_WRONLY
                         | O_APPEND 
                         | (m_options.sync_write ? O_DSYNC : 0);

    const ::std::string pathstr = m_path;
    toolpex::errret_thrower et;
    m_fd = { et << ::open(pathstr.c_str(), open_flags, create_mode) };
}

posix_writable::
posix_writable(toolpex::unique_posix_fd fd, options opt) noexcept
    : m_options{ ::std::move(opt) }, 
      m_buffer{ buffer_size_nbytes() }, 
      m_fd{ ::std::move(fd) }
{
}

posix_writable::
~posix_writable() noexcept
{
}

koios::exp_taskec<>
posix_writable::
close() noexcept
{
    co_await flush();
    co_return koios::ok();
}

koios::exp_taskec<> 
posix_writable::
sync_impl(koios::unique_lock& lk) noexcept
{
    co_await flush_valid(lk);
    if (auto ec = (co_await uring::fdatasync(m_fd)).error_code(); ec)
    {
        co_return koios::unexpected(ec);
    }
    co_return koios::ok();
}

koios::exp_taskec<> 
posix_writable::sync() noexcept
{
    if (m_options.sync_write) co_return koios::ok();
    auto lk = co_await m_mutex.acquire();
    co_return co_await sync_impl(lk);
}

koios::exp_taskec<> 
posix_writable::
append(::std::span<const ::std::byte> buffer) noexcept
{
    auto lk = co_await m_mutex.acquire();
    if (!need_buffered())
    {
        auto ec = co_await uring::append_all(m_fd, m_buffer.valid_span());
        if (ec) co_return koios::unexpected(ec);
        co_return koios::ok();
    }
    if (m_buffer.append(buffer)) // fit in
        co_return koios::ok();
    
    auto ec = co_await flush_valid(lk);
    if (!ec.has_value()) co_return koios::unexpected(ec.error());
    
    if (buffer.size_bytes() > m_buffer.capacity())
    {
        auto ec = co_await uring::append_all(m_fd, m_buffer.valid_span());
        if (ec) co_return koios::unexpected(ec);
    }
    m_buffer.append(buffer);
    co_return koios::ok();
}

koios::exp_taskec<> 
posix_writable::
append(::std::span<const char> buffer) noexcept
{
    return append(as_bytes(buffer));
}

bool posix_writable::need_buffered() const noexcept
{
    return m_options.need_buffered_write;
}

size_t posix_writable::buffer_size_nbytes() const noexcept
{
    return need_buffered() ? m_options.disk_block_bytes : 0;
}

koios::exp_taskec<> 
posix_writable::
flush() noexcept
{
    auto lk = co_await m_mutex.acquire();
    co_return co_await flush_valid(lk);
}

koios::exp_taskec<> 
posix_writable::
flush_block(koios::unique_lock& lk) noexcept
{
    ::std::error_code ret = co_await uring::append_all(
        m_fd, m_buffer.whole_span()
    );
    if (ret)
    {
        co_return koios::unexpected(ret);
    }
    else m_buffer.turncate(); // expcetion safe.
    co_return koios::ok();
}

koios::exp_taskec<> 
posix_writable::
flush_valid(koios::unique_lock& lk) noexcept
{
    ::std::error_code ret = co_await uring::append_all(
        m_fd, m_buffer.valid_span()
    );
    if (ret)
    {
        co_return koios::unexpected(ret);
    }
    else m_buffer.turncate(); // expcetion safe.
    co_return koios::ok();
}

}
