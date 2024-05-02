#ifndef FRENZYKY_POSIX_WRITABLE_H
#define FRENZYKY_POSIX_WRITABLE_H

#include "toolpex/unique_posix_fd.h"
#include "koios/task.h"
#include "koios/coroutine_mutex.h"
#include "frenzykv/kvdb_deps.h"
#include "frenzykv/io/writable.h"
#include "frenzykv/io/inner_buffer.h"
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
    static constexpr mode_t default_create_mode()
    {
        return S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    }

public:
    iouring_writable(::std::filesystem::path path, 
                     const kvdb_deps& opt, 
                     mode_t create_mode = default_create_mode(),
                     int extra_opt = 0
                    );
    iouring_writable(toolpex::unique_posix_fd fd, const kvdb_deps& opt) noexcept;

    const ::std::filesystem::path path() const noexcept { return m_path; }
    virtual koios::task<> close() override;

    virtual koios::task<size_t> append(::std::span<const ::std::byte> buffer) override;
    virtual koios::task<> sync() override;
    virtual koios::task<> flush() override;

    koios::task<size_t> append(::std::span<const char> buffer);
    virtual ::std::span<::std::byte> writable_span() noexcept override;
    virtual koios::task<> commit(size_t len) noexcept override;
    virtual outbuf_adapter* streambuf() noexcept override { return &m_streambuf; }

    uintmax_t file_size() const override;
    bool is_buffering() const noexcept override;

private:
    koios::task<> sync_impl();

private:
    bool need_buffered() const noexcept;
    size_t buffer_size_nbytes() const noexcept;
    koios::task<> flush_block();
    koios::task<> flush_valid();

private:
    const kvdb_deps* m_deps{};
    ::std::filesystem::path m_path;
    buffer<> m_buffer;
    outbuf_adapter m_streambuf;
};

} // namespace frenzykv

#endif
