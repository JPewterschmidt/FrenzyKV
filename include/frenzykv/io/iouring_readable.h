#ifndef FRENZYKV_IOURING_READABLE_H
#define FRENZYKV_IOURING_READABLE_H

#include <filesystem>

#include "frenzykv/io/readable.h"
#include "frenzykv/io/inner_buffer.h"
#include "frenzykv/kvdb_deps.h"
#include "frenzykv/posix_base.h"

#include "toolpex/unique_posix_fd.h"

namespace frenzykv
{

class iouring_readable : public posix_base, public random_readable 
{
public:
    iouring_readable(const ::std::filesystem::path& p, 
                     const kvdb_deps& deps);

    iouring_readable(toolpex::unique_posix_fd fd, 
                     const kvdb_deps& deps)
        : posix_base{ ::std::move(fd) }, 
          m_deps{ &deps }
    {
    }

    ~iouring_readable() noexcept = default;

    koios::task<size_t>
    read(::std::span<::std::byte> dest) override;

    koios::task<size_t> 
    read(::std::span<::std::byte>, size_t offset) const override;

    constexpr bool is_buffering() const noexcept override { return false; }

private:
    const kvdb_deps* m_deps;
    seq_readable_context m_fdctx;
};

}

#endif
