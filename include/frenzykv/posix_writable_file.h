#ifndef FRENZYKY_POSIX_WRITABLE_FILE_H
#define FRENZYKY_POSIX_WRITABLE_FILE_H

#include "toolpex/unique_posix_fd.h"
#include "koios/task.h"
#include "koios/coroutine_mutex.h"
#include "frenzykv/options.h"
#include "frenzykv/writable.h"
#include "frenzykv/inner_buffer.h"

#include <span>
#include <memory>
#include <filesystem>

namespace frenzykv
{

class posix_writable_file : public seq_writable
{
public:
    posix_writable_file(::std::filesystem::path path, options opt = get_global_options());
    virtual ~posix_writable_file() noexcept {}
    const ::std::filesystem::path path() const noexcept { return m_path; }
    virtual void close() noexcept override { m_fd.close(); }; 

    virtual koios::task<::std::error_code> append(::std::span<const ::std::byte> buffer) noexcept override;
    virtual koios::task<::std::error_code> sync() noexcept override;
    virtual koios::task<::std::error_code> flush() noexcept override;

    koios::task<::std::error_code> append(::std::span<const char> buffer) noexcept;

private:
    koios::task<::std::error_code> sync_impl(koios::unique_lock&) noexcept;

private:
    bool need_buffered() const noexcept;
    size_t buffer_size_nbytes() const noexcept;
    koios::task<::std::error_code> flush_block(koios::unique_lock& lk) noexcept;
    koios::task<::std::error_code> flush_valid(koios::unique_lock& lk) noexcept;

private:
    options m_options;
    ::std::filesystem::path m_path;
    detials::buffer<> m_buffer;
    toolpex::unique_posix_fd m_fd;
    koios::mutex m_mutex;
};

} // namespace frenzykv

#endif
