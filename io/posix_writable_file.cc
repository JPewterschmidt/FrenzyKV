#include "frenzykv/posix_writable_file.h"
#include "frenzykv/error_category.h"
#include "toolpex/errret_thrower.h"
#include "koios/iouring_awaitables.h"
#include <fcntl.h>

namespace frenzykv
{

using namespace koios;

posix_writable_file::
posix_writable_file(::std::filesystem::path path, options opt)
    : m_options{ ::std::move(opt) }, 
      m_path{ ::std::move(path) }, 
      m_buffer{ buffer_size_nbytes() }
{
    const int open_flags = O_CREAT 
                         | O_APPEND 
                         | (m_options.sync_write ? O_DSYNC : 0);

    const ::std::string pathstr = m_path;
    toolpex::errret_thrower et;
    m_fd = { et << ::open(pathstr.c_str(), open_flags) };
}

koios::task<::std::error_code> 
posix_writable_file::
sync_impl(koios::unique_lock& lk) noexcept
{
    ::std::error_code ec = co_await uring::append_all(
        m_fd, m_buffer.valid_span()
    );
    if (ec) co_return ec;
    co_return (co_await uring::fdatasync(m_fd)).error_code();
}

koios::task<::std::error_code> 
posix_writable_file::sync() noexcept
{
    if (m_options.sync_write) co_return make_frzkv_ok();
    auto lk = co_await m_mutex.acquire();
    co_return co_await sync_impl(lk);
}

koios::task<::std::error_code> 
posix_writable_file::
append(::std::span<const ::std::byte> buffer) noexcept
{
    auto lk = co_await m_mutex.acquire();
    if (!need_buffered())
    {
        co_return co_await uring::append_all(m_fd, m_buffer.valid_span());
    }
    if (m_buffer.append(buffer)) // fit in
        co_return make_frzkv_ok();
    
    auto ec = co_await flush_valid(lk);
    if (ec) co_return ec;
    
    if (buffer.size_bytes() > m_buffer.capacity())
    {
        co_return co_await uring::append_all(m_fd, m_buffer.valid_span());
    }
    m_buffer.append(buffer);
    co_return make_frzkv_ok();
}

koios::task<::std::error_code> 
posix_writable_file::
append(::std::span<const char> buffer) noexcept
{
    return append(as_bytes(buffer));
}

bool posix_writable_file::need_buffered() const noexcept
{
    return m_options.need_buffered_write;
}

size_t posix_writable_file::buffer_size_nbytes() const noexcept
{
    return need_buffered() ? m_options.disk_block_bytes : 0;
}

koios::task<::std::error_code> 
posix_writable_file::
flush() noexcept
{
    auto lk = co_await m_mutex.acquire();
    co_return co_await flush_valid(lk);
}

koios::task<::std::error_code> 
posix_writable_file::
flush_block(koios::unique_lock& lk) noexcept
{
    ::std::error_code ret = co_await uring::append_all(
        m_fd, m_buffer.whole_span()
    );
    if (!ret) m_buffer.turncate(); // expcetion safe.
    co_return ret;
}

koios::task<::std::error_code> 
posix_writable_file::
flush_valid(koios::unique_lock& lk) noexcept
{
    ::std::error_code ret = co_await uring::append_all(
        m_fd, m_buffer.valid_span()
    );
    if (!ret) m_buffer.turncate();
    co_return ret;
}

}
