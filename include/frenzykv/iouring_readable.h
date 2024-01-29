#ifndef FRENZYKV_IOURING_READABLE_H
#define FRENZYKV_IOURING_READABLE_H

#include <filesystem>

#include "frenzykv/readable.h"

#include "toolpex/unique_posix_fd.h"

namespace frenzykv
{

class iouring_readable : public random_readable, private seq_readable_context
{
public:
    iouring_readable(const ::std::filesystem::path& p);
    iouring_readable(toolpex::unique_posix_fd fd) noexcept
        : m_fd{ ::std::move(fd) }
    {
    }

    ~iouring_readable() noexcept = default;

    koios::task<size_t>
    read(::std::span<::std::byte> dest) override;

    koios::task<size_t> 
    read(::std::span<::std::byte>, size_t offset) const override;

private:
    toolpex::unique_posix_fd m_fd{};
};

}

#endif
