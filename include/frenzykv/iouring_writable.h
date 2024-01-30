#ifndef FRENZYKY_POSIX_WRITABLE_H
#define FRENZYKY_POSIX_WRITABLE_H

#include "toolpex/unique_posix_fd.h"
#include "koios/task.h"
#include "koios/coroutine_mutex.h"
#include "frenzykv/options.h"
#include "frenzykv/writable.h"
#include "frenzykv/inner_buffer.h"
#include "frenzykv/posix_base.h"

#include <span>
#include <memory>
#include <filesystem>
#include <sys/types.h>

namespace frenzykv
{

class iouring_writable : public posix_base, public seq_writable
{
public:
    iouring_writable(::std::filesystem::path path, 
                     options opt = get_global_options(), 
                     mode_t create_mode = 
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    iouring_writable(toolpex::unique_posix_fd fd, options opt = get_global_options()) noexcept;
    virtual ~iouring_writable() noexcept override;
    const ::std::filesystem::path path() const noexcept { return m_path; }
    virtual koios::task<> close() override;

    virtual koios::task<size_t> append(::std::span<const ::std::byte> buffer) override;
    virtual koios::task<> sync() override;
    virtual koios::task<> flush() override;

    koios::task<size_t> append(::std::span<const char> buffer);
    ::std::span<::std::byte> writable_span() noexcept override;
    koios::task<> commit(size_t len) noexcept override;

private:
    koios::task<> sync_impl();

private:
    bool need_buffered() const noexcept;
    size_t buffer_size_nbytes() const noexcept;
    koios::task<> flush_block();
    koios::task<> flush_valid();

private:
    options m_options;
    ::std::filesystem::path m_path;
    detials::buffer<> m_buffer;
};

} // namespace frenzykv

#endif
