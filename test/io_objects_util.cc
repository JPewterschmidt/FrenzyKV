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

} // annoymous namespace

TEST(io_objects_util, io)
{
    in_mem_rw source_file = make_source_file().result();
    ASSERT_TRUE(test_body(source_file).result());
}
