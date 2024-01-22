#include "frenzykv/posix_writable.h"
#include "frenzykv/error_category.h"
#include "toolpex/errret_thrower.h"
#include "koios/iouring_awaitables.h"
#include <fcntl.h>
#include <cassert>

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

koios::task<>
posix_writable::
close()
{
    co_await flush();
    m_fd.close();
}

koios::task<>
posix_writable::
sync_impl(koios::unique_lock& lk) 
{
    co_await flush_valid(lk);
    co_await uring::fdatasync(m_fd);
}

koios::task<>
posix_writable::sync()
{
    if (m_options.sync_write) 
    {
        auto lk = co_await m_mutex.acquire();
        co_await sync_impl(lk);
    }
}

koios::task<size_t>
posix_writable::
append(::std::span<const ::std::byte> buffer) 
{
    auto lk = co_await m_mutex.acquire();
    if (!need_buffered())
    {
        co_await uring::append_all(m_fd, m_buffer.valid_span());
        co_return buffer.size_bytes();
    }
    if (m_buffer.append(buffer)) // fit in
        co_return buffer.size_bytes();
    
    co_await flush_valid(lk);
    
    if (buffer.size_bytes() > m_buffer.capacity())
    {
        co_await uring::append_all(m_fd, m_buffer.valid_span());
    }
    if (!m_buffer.append(buffer))
        throw koios::exception{"posix_writable append error"};
    co_return buffer.size_bytes();
}

koios::task<size_t>
posix_writable::
append(::std::span<const char> buffer)
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

koios::task<>
posix_writable::
flush()
{
    auto lk = co_await m_mutex.acquire();
    co_await flush_valid(lk);
}

koios::task<>
posix_writable::
flush_block(koios::unique_lock& lk) 
{
    co_await uring::append_all(m_fd, m_buffer.whole_span());
    m_buffer.turncate();
}

koios::task<>
posix_writable::
flush_valid(koios::unique_lock& lk)
{
    co_await uring::append_all(m_fd, m_buffer.valid_span());
    m_buffer.turncate();
}

}
