#ifndef FRENZYKV_IOURING_READABLE_H
#define FRENZYKV_IOURING_READABLE_H

#include <filesystem>
#include <string>
#include <string_view>

#include "frenzykv/io/readable.h"
#include "frenzykv/io/inner_buffer.h"
#include "frenzykv/options.h"
#include "frenzykv/posix_base.h"

#include "toolpex/unique_posix_fd.h"

namespace frenzykv
{

class iouring_readable : public posix_base, public random_readable 
{
public:
    iouring_readable(const ::std::filesystem::path& p, 
                     const options& opt);

    ~iouring_readable() noexcept = default;

    koios::task<size_t>
    read(::std::span<::std::byte> dest) override;

    koios::task<size_t> 
    read(::std::span<::std::byte>, size_t offset) const override;

    koios::task<> close() noexcept override { co_return; }

    constexpr bool is_buffering() const noexcept override { return false; }

    uintmax_t file_size() const override { return m_filesize; }

    ::std::string_view filename() const noexcept override { return m_filename; }

private:
    const options* m_opt;
    seq_readable_context m_fdctx;
    uintmax_t m_filesize{};
    ::std::string m_filename;
};

}

#endif
