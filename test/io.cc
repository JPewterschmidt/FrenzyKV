#include "gtest/gtest.h"

#include <string>
#include <ranges>
#include <filesystem>

#include "koios/iouring_awaitables.h"

#include "frenzykv/io/io_objects_util.h"
#include "frenzykv/io/iouring_writable.h"
#include "frenzykv/types.h"

using namespace frenzykv;
namespace rv = ::std::ranges::views;
namespace fs = ::std::filesystem;

namespace
{

::std::vector<fs::path> opened_files;
kvdb_deps deps;

koios::task<::std::unique_ptr<in_mem_rw>> make_in_mem_source_file()
{
    auto result = ::std::make_unique<in_mem_rw>();
    ::std::string buffer(40960, 1);
    const_bspan cb{ ::std::as_bytes(::std::span{ buffer }) };
    co_await result->append(cb);
    co_return result;
}

koios::eager_task<::std::unique_ptr<iouring_writable>> make_real_file()
{
    static size_t count{};
    ::std::string filename = ::std::format("testfile{}", count++);
    const auto& p = opened_files.emplace_back(filename);
    auto optp = deps.opt();
    co_return ::std::make_unique<iouring_writable>(p, *optp);
}

koios::task<bool> in_mem_test_body(random_readable& file)
{
    in_mem_rw new_file = co_await to_in_mem_rw(file);
    co_return new_file.file_size() == 40960;
}

koios::task<bool> in_mem_dump_test()
{
    auto src = co_await make_in_mem_source_file();
    const size_t src_size = src->file_size();
    in_mem_rw dest;

    if (dest.file_size() != 0)
        co_return false;

    size_t wrote = co_await src->dump_to(dest);
    if (wrote != src_size) 
        co_return false;

    if (dest.file_size() != src_size)
        co_return false;

    if (src->file_size() != src_size)
        co_return false;

    co_return true;
}

koios::eager_task<> delete_file()
{
    for (auto p : opened_files)
    {
        co_await koios::uring::unlink(p);
    }
}

koios::eager_task<bool> real_file_test()
{
    auto file = co_await make_real_file();
    assert(file->file_size() == 0);
    auto inmem = co_await make_in_mem_source_file();
    co_await inmem->dump_to(*file);
    co_await file->sync();
    co_return file->file_size() == inmem->file_size();
}

} // annoymous namespace

TEST(in_mem, io)
{
    auto source_file = make_in_mem_source_file().result();
    ASSERT_TRUE(in_mem_test_body(*source_file).result());
}

TEST(in_mem, in_mem_dump)
{
    ASSERT_TRUE(in_mem_dump_test().result());
}

TEST(real_file, basic)
{
    ASSERT_TRUE(real_file_test().result());
    delete_file().result();   
}

TEST(real_file, delete_test_file)
{
    delete_file().result();
}
