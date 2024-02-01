#ifndef FRENZYKV_IOURING_READABLE_H
#define FRENZYKV_IOURING_READABLE_H

#include <filesystem>

#include "frenzykv/readable.h"
#include "frenzykv/options.h"
#include "frenzykv/posix_base.h"
#include "frenzykv/inner_buffer.h"

#include "toolpex/unique_posix_fd.h"

namespace frenzykv
{

class iouring_readable : public posix_base, public random_readable 
{
public:
    iouring_readable(const ::std::filesystem::path& p, 
                     options opt = get_global_options());

    iouring_readable(toolpex::unique_posix_fd fd, 
                     options opt = get_global_options())
        : posix_base{ ::std::move(fd) }, 
          m_opt{ ::std::move(opt) }
    {
    }

    ~iouring_readable() noexcept = default;

    koios::task<size_t>
    read(::std::span<::std::byte> dest) override;

    koios::task<size_t> 
    read(::std::span<::std::byte>, size_t offset) const override;

    constexpr bool is_buffering() const noexcept override { return false; }

private:
    options m_opt;
    seq_readable_context m_fdctx;
};

}

#endif
