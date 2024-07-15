// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <fcntl.h>
#include <sys/stat.h>

#include "toolpex/errret_thrower.h"

#include "koios/iouring_awaitables.h"
#include "koios/this_task.h"
#include "koios/task.h"

#include "frenzykv/io/iouring_readable.h"

using namespace koios;

namespace frenzykv
{

static toolpex::errret_thrower et;

iouring_readable::iouring_readable(const ::std::filesystem::path& p, const options& opt)
    : posix_base{ et << ::open(p.c_str(), O_CREAT | O_RDONLY | O_CLOEXEC, file::default_create_mode()) }, 
      m_opt{ &opt }, m_filename{ p.filename().string() }
{
    typename ::stat st{};
    et << ::fstat(fd(), &st);
    m_filesize = static_cast<uintmax_t>(st.st_size);
}

koios::task<size_t>
iouring_readable::read(::std::span<::std::byte> dest)
{
    co_return m_fdctx.has_read((co_await uring::read(m_fd, dest, m_fdctx.cursor())).nbytes_delivered());
}

koios::task<size_t> 
iouring_readable::read(::std::span<::std::byte> dest, size_t offset) const
{
    co_return (co_await uring::read(m_fd, dest, offset)).nbytes_delivered();
}

koios::task<size_t> 
iouring_readable::dump_to(seq_writable& file)
{
    const size_t file_sz = file_size();
    size_t wrote{};
    do
    {
        auto buffer = file.writable_span();
        const size_t each_read = co_await read(buffer, wrote);
        co_await file.commit(each_read);
        wrote += each_read;
    }
    while (file_sz - wrote);

    co_return wrote;
}

} // namespace frenzykv
