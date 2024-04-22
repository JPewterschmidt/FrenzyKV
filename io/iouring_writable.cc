#include "frenzykv/io/iouring_writable.h"
#include "frenzykv/error_category.h"
#include "toolpex/errret_thrower.h"
#include "koios/iouring_awaitables.h"
#include <fcntl.h>
#include <cassert>

namespace frenzykv
{

using namespace koios;

static toolpex::errret_thrower et;

static toolpex::unique_posix_fd
open_helper(const auto& opts, const ::std::string& pathstr, mode_t create_mode)
{
    const int open_flags = O_CREAT 
                         | O_WRONLY
                         | O_APPEND 
                         | (opts.sync_write ? O_DSYNC : 0);

    return { et << ::open(pathstr.c_str(), open_flags, create_mode) };
}

iouring_writable::
iouring_writable(::std::filesystem::path path, const kvdb_deps& deps, mode_t create_mode)
    : posix_base{ open_helper(*deps.opt(), path, create_mode) }, 
      m_deps{ &deps }, 
      m_path{ ::std::move(path) }, 
      m_buffer{ buffer_size_nbytes() }, 
      m_streambuf{ m_buffer }
{
}

iouring_writable::
iouring_writable(toolpex::unique_posix_fd fd, const kvdb_deps& deps) noexcept
    : posix_base{ ::std::move(fd) },
      m_deps{ &deps }, 
      m_buffer{ buffer_size_nbytes() }, 
      m_streambuf{ m_buffer }
{
}

uintmax_t iouring_writable::file_size() const
{
    typename ::stat st{};
    et << ::fstat(fd(), &st);
    return st.st_size;
}

bool iouring_writable::is_buffering() const noexcept
{
    return need_buffered();
}

koios::task<>
iouring_writable::
close()
{
    co_await flush();
    m_fd.close();
}

koios::task<>
iouring_writable::
sync_impl() 
{
    co_await flush_valid();
    co_await uring::fdatasync(m_fd);
}

koios::task<>
iouring_writable::sync()
{
    co_await sync_impl();
}

koios::task<size_t>
iouring_writable::
append(::std::span<const ::std::byte> buffer) 
{
    if (!need_buffered())
    {
        co_await uring::append_all(m_fd, m_buffer.valid_span());
        co_return buffer.size_bytes();
    }
    if (m_buffer.append(buffer)) // fit in
    {
        if (m_buffer.full()) co_await flush_valid();
        co_return buffer.size_bytes();
    }
    
    co_await flush_valid();
    
    if (buffer.size_bytes() > m_buffer.capacity())
    {
        co_await uring::append_all(m_fd, m_buffer.valid_span());
    }
    if (!m_buffer.append(buffer))
        throw koios::exception{"iouring_writable append error"};
    co_return buffer.size_bytes();
}

koios::task<size_t>
iouring_writable::
append(::std::span<const char> buffer)
{
    return append(as_bytes(buffer));
}

bool iouring_writable::need_buffered() const noexcept
{
    return m_deps->opt()->need_buffered_write;
}

size_t iouring_writable::buffer_size_nbytes() const noexcept
{
    return need_buffered() ? m_deps->opt()->disk_block_bytes : 0;
}

koios::task<>
iouring_writable::
flush()
{
    co_await flush_valid();
}

koios::task<>
iouring_writable::
flush_block() 
{
    co_await uring::append_all(m_fd, m_buffer.whole_span());
    m_buffer.turncate();
    m_streambuf.update_state();
}

koios::task<>
iouring_writable::
flush_valid()
{
    auto sp = m_buffer.valid_span();
    if (sp.empty()) co_return;
    co_await uring::append_all(m_fd, sp);
    m_buffer.turncate();
    m_streambuf.update_state();
}

::std::span<::std::byte> 
iouring_writable::
writable_span() noexcept
{
    return m_buffer.writable_span();
}

koios::task<> 
iouring_writable::
commit(size_t len) noexcept
{
    m_buffer.commit(len);
    if (m_buffer.full())
    {
        co_await flush_valid();
    }
    m_streambuf.update_state();
    co_return;
}

}
