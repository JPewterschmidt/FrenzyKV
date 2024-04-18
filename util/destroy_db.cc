#include "frenzykv/destroy_db.h"
#include "toolpex/errret_thrower.h"
#include "koios/iouring_awaitables.h"

namespace frenzykv
{

static toolpex::errret_thrower et;

namespace fs = ::std::filesystem;
using namespace ::std::chrono_literals;

koios::eager_task<> file_grinder(fs::directory_entry entry)
{
    const auto path = entry.path();
    try
    {
        toolpex::unique_posix_fd fd{ et << ::open(path.c_str(), O_RDWR) };
        size_t wrote{};
        const auto sz = entry.file_size();
        ::std::array<char, 4096> valbuffer{};
        while (wrote < sz)
        {
            ::std::span<char> val{ valbuffer };
            const auto left = sz - wrote;
            const auto ioret = co_await koios::uring::write(
                fd, val.subspan(0, left > val.size() ? val.size() : left), 
                wrote, 5s);
            const auto this_wrote = ioret.nbytes_delivered();
            wrote += this_wrote;
            if (this_wrote == 4096)
            {
                co_await koios::uring::fsync(fd);
            }
        }
        co_await koios::uring::fsync(fd);
    }
    catch (...)
    {
        // TODO: log
    }

    co_await koios::uring::unlink(path);

    co_return;
}

void destroy_db(::std::filesystem::path p)
{
    for (auto de : fs::recursive_directory_iterator{ p })
    {
        if (de.is_regular_file())
        {
            file_grinder(::std::move(de)).run();
        }
    }
}

} // namespace frenzykv
