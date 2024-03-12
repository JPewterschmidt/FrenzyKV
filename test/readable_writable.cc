#include "gtest/gtest.h"
#include "frenzykv/readable.h"
#include "frenzykv/writable.h"
#include "frenzykv/in_mem_rw.h"
#include "frenzykv/iouring_writable.h"
#include "koios/iouring_awaitables.h"

#include <fstream>

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;

namespace
{
    in_mem_rw r{3};
    ::std::string filename = "testfile.txt";
    iouring_writable w{filename};
    
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
}

TEST(readable_test_env, basic)
{
    ASSERT_TRUE(env_setup().result());
    ASSERT_TRUE(testbody_in_mem_rw().result());
    ASSERT_TRUE(testbody_posix().result());
}
