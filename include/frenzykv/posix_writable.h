#ifndef FRENZYKY_POSIX_WRITABLE_H
#define FRENZYKY_POSIX_WRITABLE_H

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

class posix_writable : public seq_writable
{
public:
    posix_writable(::std::filesystem::path path, options opt = get_global_options());
    posix_writable(toolpex::unique_posix_fd fd, options opt = get_global_options()) noexcept;
    virtual ~posix_writable() noexcept override;
    const ::std::filesystem::path path() const noexcept { return m_path; }
    virtual koios::taskec close() noexcept override;

    virtual koios::taskec append(::std::span<const ::std::byte> buffer) noexcept override;
    virtual koios::taskec sync() noexcept override;
    virtual koios::taskec flush() noexcept override;

    koios::taskec append(::std::span<const char> buffer) noexcept;

private:
    koios::taskec sync_impl(koios::unique_lock&) noexcept;

private:
    bool need_buffered() const noexcept;
    size_t buffer_size_nbytes() const noexcept;
    koios::taskec flush_block(koios::unique_lock& lk) noexcept;
    koios::taskec flush_valid(koios::unique_lock& lk) noexcept;

private:
    options m_options;
    ::std::filesystem::path m_path;
    detials::buffer<> m_buffer;
    toolpex::unique_posix_fd m_fd;
    koios::mutex m_mutex;
};

} // namespace frenzykv

#endif
