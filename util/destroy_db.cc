#include "frenzykv/destroy_db.h"
#include "toolpex/errret_thrower.h"
#include "koios/iouring_awaitables.h"
#include "spdlog/spdlog.h"

namespace frenzykv
{

static toolpex::errret_thrower et;

namespace fs = ::std::filesystem;
using namespace ::std::chrono_literals;

koios::eager_task<> file_grinder(fs::directory_entry entry)
try
{
    const auto path = entry.path();
    toolpex::unique_posix_fd fd{ et << ::open(path.c_str(), O_RDWR) };

    // Unlink after the file was opened, will make it invisiable at the directory.
    co_await koios::uring::unlink(path); 

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
catch (const koios::exception& e)
{
    e.log();   
    co_return;
}
catch (...)
{
    spdlog::error("something wrong during the DB destroying phase.");
    co_return;
}

void destroy_db(::std::filesystem::path p, ::std::string_view caller_declaration)
{
    static ::std::string_view right_decl = "I know exactly what I'm doing, just destroy the DB unrecoverably right now!"; 
    if (caller_declaration != right_decl) 
        throw koios::exception{ R"(If you want to destroy the DB, you need pass a right caller declaration:"I know exactly what I'm doing, just destroy the DB unrecoverably right now!")" };
        
    spdlog::info("File under {} start deleting parallely, "
                 "This function call would return immediately, "
                 "you may want to execute `sync` several time "
                 "to insure the deleting completely.", 
                 p.string());

    for (auto de : fs::recursive_directory_iterator{ p })
    {
        if (de.is_regular_file())
        {
            file_grinder(::std::move(de)).run();
        }
    }
}

} // namespace frenzykv
