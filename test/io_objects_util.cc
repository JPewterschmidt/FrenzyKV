#include "gtest/gtest.h"

#include <string>

#include "frenzykv/io/io_objects_util.h"
#include "frenzykv/types.h"

using namespace frenzykv;

namespace
{

koios::task<in_mem_rw> make_source_file()
{
    in_mem_rw result{};
    ::std::string buffer(128, 1);
    const_bspan cb{ ::std::as_bytes(::std::span{ buffer }) };
    co_await result.append(cb);
    co_return result;
}

koios::task<bool> test_body(random_readable& file)
{
    in_mem_rw new_file = co_await to_in_mem_rw(file);
    co_return new_file.file_size() == 128;
}

koios::task<bool> in_mem_dump_test()
{
    in_mem_rw src = co_await make_source_file();
    const size_t src_size = src.file_size();
    in_mem_rw dest;

    if (dest.file_size() != 0)
        co_return false;

    size_t wrote = co_await src.dump_to(dest);
    if (wrote != src_size) 
        co_return false;

    if (dest.file_size() != src_size)
        co_return false;

    if (src.file_size() != src_size)
        co_return false;

    co_return true;
}

} // annoymous namespace

TEST(io_objects_util, io)
{
    in_mem_rw source_file = make_source_file().result();
    ASSERT_TRUE(test_body(source_file).result());
}

TEST(io_objects_util, in_mem_dump)
{
    ASSERT_TRUE(in_mem_dump_test().result());
}
