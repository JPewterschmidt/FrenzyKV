#include "gtest/gtest.h"
#include "frenzykv/readable.h"
#include "frenzykv/writable.h"
#include "frenzykv/in_mem_rw.h"
#include "frenzykv/posix_writable.h"
#include "koios/iouring_awaitables.h"
#include "koios/iouring_unlink_aw.h"

#include <fstream>

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;

namespace
{
    in_mem_rw r{3};
    ::std::string filename = "testfile.txt";
    posix_writable w{filename};
    
    task<void> env_setup()
    {
        seq_writable& w = r;
        co_await w.append("123456789abcdefghijk"sv);
    }

    task<bool> testbody_in_mem_rw()
    {
        ::std::array<char, 5> buffer{};
        ::std::span sp{ buffer.begin(), buffer.end() };
        
        ::std::error_code ec;
        
        seq_readable& ref = r;

        ec = co_await ref.read(as_writable_bytes(sp));
        if (ec) co_return false;

        ec = co_await ref.read(as_writable_bytes(sp));
        if (ec) co_return false;

        ec = co_await ref.read(as_writable_bytes(sp));
        if (ec) co_return false;

        co_return ::std::memcmp(buffer.data(), "bcdef", 5) == 0;
    }

    task<bool> testbody_posix()
    {
        seq_writable& ref = w;
        ::std::string test_txt = "1234567890abcdefg\n";
        ::std::error_code ec;

        ec = co_await ref.append(test_txt);
        if (ec) co_return false;
        ec = co_await ref.append(test_txt);
        if (ec) co_return false;

        ec = co_await ref.flush();
        ec = co_await ref.close();
        if (ec) co_return false;

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
}

TEST(readable_test_env, basic)
{
    env_setup().result();
    ASSERT_TRUE(testbody_in_mem_rw().result());
    ASSERT_TRUE(testbody_posix().result());
}
