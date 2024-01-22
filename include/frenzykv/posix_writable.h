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
#include <sys/types.h>

namespace frenzykv
{

class posix_writable : public seq_writable
{
public:
    posix_writable(::std::filesystem::path path, 
                   options opt = get_global_options(), 
                   mode_t create_mode = 
                       S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    posix_writable(toolpex::unique_posix_fd fd, options opt = get_global_options()) noexcept;
    virtual ~posix_writable() noexcept override;
    const ::std::filesystem::path path() const noexcept { return m_path; }
    virtual koios::exp_taskec<> close() noexcept override;

    virtual koios::exp_taskec<> append(::std::span<const ::std::byte> buffer) noexcept override;
    virtual koios::exp_taskec<> sync() noexcept override;
    virtual koios::exp_taskec<> flush() noexcept override;

    koios::exp_taskec<> append(::std::span<const char> buffer) noexcept;

private:
    koios::exp_taskec<> sync_impl(koios::unique_lock&) noexcept;

private:
    bool need_buffered() const noexcept;
    size_t buffer_size_nbytes() const noexcept;
    koios::exp_taskec<> flush_block(koios::unique_lock& lk) noexcept;
    koios::exp_taskec<> flush_valid(koios::unique_lock& lk) noexcept;

private:
    options m_options;
    ::std::filesystem::path m_path;
    detials::buffer<> m_buffer;
    toolpex::unique_posix_fd m_fd;
    koios::mutex m_mutex;
};

} // namespace frenzykv

#endif
