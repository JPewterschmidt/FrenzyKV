#include "util/flock.h"
#include "gtest/gtest.h"
#include "koios/task.h"
#include "koios/iouring_awaitables.h"
#include "toolpex/errret_thrower.h"

using namespace frenzykv;
using namespace koios;
using namespace toolpex;

namespace
{
    static constexpr ::std::string g_file_name = "testflock.txt";

    task<toolpex::unique_posix_fd> create_file()
    {
        toolpex::errret_thrower et;
        co_return et << ::creat(g_file_name.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    }

    task<> delete_file()
    {
        co_await uring::unlink(g_file_name);
    }
}

TEST(flock, basic)
{
    auto fd = create_file().result();

    auto flkh = try_flock_shared(fd);   
    ASSERT_TRUE(flkh.has_value());
    ASSERT_TRUE(try_flock_shared(fd).has_value());
    ASSERT_TRUE(try_flock_unique(fd).has_value());
    flkh = ::std::nullopt;

    auto fl_unique_h = try_flock_unique(fd);
    ASSERT_TRUE(fl_unique_h.has_value());
    ASSERT_TRUE(try_flock_shared(fd).has_value());
    fl_unique_h = ::std::nullopt;
    fd.close();

    delete_file().result();
}

namespace
{
    task<bool> aw_test_flock()
    try
    {
        auto fd = co_await create_file();
        proc_flock_async_hub hub(fd);
        auto h = co_await hub.acquire_shared();
        auto h2 = co_await hub.acquire_unique();
        co_return true;
    }
    catch (...)
    {
        co_return false;
    }
}

TEST(flock, aw)
{
    ASSERT_TRUE(aw_test_flock().result());
    delete_file().result();
}
