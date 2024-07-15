// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "gtest/gtest.h"

#include <string>
#include <ranges>
#include <fstream>
#include <filesystem>

#include "koios/iouring_awaitables.h"

#include "frenzykv/types.h"
#include "frenzykv/kvdb_deps.h"

#include "frenzykv/io/readable.h"
#include "frenzykv/io/writable.h"
#include "frenzykv/io/in_mem_rw.h"
#include "frenzykv/io/iouring_writable.h"
#include "frenzykv/io/iouring_readable.h"
#include "frenzykv/io/io_objects_util.h"

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;
using namespace ::std::string_literals;
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
    co_return ::std::make_unique<iouring_writable>(p, *optp, file::default_create_mode(), O_TRUNC);
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

namespace
{
    in_mem_rw r{3};
    ::std::string filename = "testfile.txt";
    auto optp = deps.opt();
    iouring_writable w{filename, *optp};
    
    eager_task<bool> env_setup()
    {
        seq_writable& w = r;
        const auto str = "123456789abcdefghijk"sv;
        size_t count{};
        count += co_await w.append(str);
        count += co_await w.append(str);
        count += co_await w.append(str);
        count += co_await w.append(str);
        count += co_await w.append(str);
        count += co_await w.append(str);
        co_return str.size() * 6 == count;
    }

    eager_task<bool> testbody_in_mem_rw()
    {
        ::std::array<char, 5> buffer{};
        ::std::span sp{ buffer.begin(), buffer.end() };
        
        seq_readable& ref = r;
        bool partial_result{ true };
        partial_result &= co_await ref.read(as_writable_bytes(sp)) == 5;
        partial_result &= co_await ref.read(as_writable_bytes(sp)) == 5;
        partial_result &= co_await ref.read(as_writable_bytes(sp)) == 5;

        co_return ::std::memcmp(buffer.data(), "bcdef", 5) == 0 && partial_result;
    }

    eager_task<bool> testbody_posix()
    {
        seq_writable& ref = w;
        ::std::string test_txt = "1234567890abcdefg\n";

        co_await ref.append(test_txt);
        co_await ref.append(test_txt);
        co_await ref.flush();
        co_await ref.close();

        {
            ::std::string dummy;
            ::std::ifstream ifs{ filename };
            while (getline(ifs, dummy))
            {
                if (dummy + "\n" != test_txt) co_return false;
            }
        }

        auto ret = co_await uring::unlink(filename);
        if (ret.error_code()) co_return false;

        co_return true;
    }

    eager_task<bool> iouring_readable_dump_to()
    {
        auto opt = deps.opt();
        const ::std::string test_filename = "testfile-iouring_readable_dump_to";
        iouring_writable w(test_filename, *opt, file::default_create_mode(), O_TRUNC);
        ::std::array<::std::byte, 8193> buffer{};
        ::std::memset(buffer.data(), 1, buffer.size());
        co_await w.append(buffer);
        const uintmax_t file_size = w.file_size();
        co_await w.close();
        
        in_mem_rw mem;
        iouring_readable r(test_filename, *opt);
        co_await r.dump_to(mem);
        const bool result = mem.file_size() == file_size;
        co_await koios::uring::unlink(test_filename);
        co_return result;
    }
}

TEST(readable_test_env, basic)
{
    ASSERT_TRUE(env_setup().result());
    ASSERT_TRUE(testbody_in_mem_rw().result());
    ASSERT_TRUE(testbody_posix().result());
}

TEST(real_file, iouring_readable_dump_to)
{
    ASSERT_TRUE(iouring_readable_dump_to().result());
}
