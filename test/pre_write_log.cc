#include "gtest/gtest.h"
#include "frenzykv/log/logger.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/env.h"
#include "koios/iouring_awaitables.h"
#include "toolpex/errret_thrower.h"
#include <string>

using namespace frenzykv;
using namespace ::std::string_literals;
using namespace ::std::string_view_literals;

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
        co_await l.insert(w);
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
    ::std::array<::std::byte, 1024> buffer{};
    co_await koios::uring::read(fd, ::std::as_writable_bytes(::std::span{buffer}));
    size_t first_entey_sz = serialized_entry_size(buffer.data());

    kv_entry entry1{ buffer };
    bool result = (entry1.key().user_key() == "xxxx"sv);

    kv_entry entry2{ ::std::span{ buffer }.subspan(first_entey_sz) };
    result &= (entry2.key().user_key() == "aaaa"sv);

    co_return result;
}

koios::eager_task<> clean()
{
    co_await koios::uring::unlink("pre_write_test.txt");
}

} // annoymous namespace

TEST(pre_write_log, basic)
{
    kvdb_deps deps;
    logger l(deps, deps.env()->get_seq_writable("pre_write_test.txt"));
    ASSERT_TRUE(write(l).result());
    ASSERT_TRUE(read().result());

    clean().result();
}
