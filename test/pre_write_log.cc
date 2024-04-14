#include "gtest/gtest.h"
#include "frenzykv/log/logger.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/env.h"
#include "koios/iouring_awaitables.h"
#include "toolpex/errret_thrower.h"
#include <string>

using namespace frenzykv;
using namespace ::std::string_literals;

namespace
{

write_batch make_batch()
{
    write_batch result;
    result.write("xxxx", "abc");
    result.write("aaaa", "def");
    result.set_first_sequence_num(0);

    return result;
}

koios::task<bool> write(logger& l)
{
    auto w = make_batch();
    try
    {
        co_await l.write(w);
    }
    catch (...)
    {
        co_return false;
    }
    co_return true;
}

koios::task<toolpex::unique_posix_fd> open_file()
{
    toolpex::errret_thrower et;
    co_return et << ::open("pre_write_test.txt", O_RDWR);
}

koios::eager_task<bool> read()
{
    auto fd = co_await open_file();
    ::std::array<char, 1024> buffer{};
    co_await koios::uring::read(fd, ::std::as_writable_bytes(::std::span{buffer}));
    entry_pbrep e;
    e.ParseFromArray(buffer.data(), buffer.size());
    co_return e.key().user_key() == "aaaa"s;
}

koios::eager_task<> clean()
{
    co_await koios::uring::unlink("pre_write_test.txt");
}

} // annoymous namespace

TEST(pre_write_log, basic)
{
    kvdb_deps deps;
    logger l(deps, "pre_write_test.txt");
    ASSERT_TRUE(write(l).result());
    ASSERT_TRUE(read().result());

    clean().result();
}
