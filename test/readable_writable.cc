#include "gtest/gtest.h"
#include "frenzykv/readable.h"
#include "frenzykv/writable.h"
#include "frenzykv/in_mem_rw.h"

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;

namespace
{
    in_mem_rw r{3};
    task<void> env_setup()
    {
        seq_writable& w = r;
        co_await w.append("123456789abcdefghijk"sv);
    }

    task<bool> testbody()
    {
        ::std::array<char, 5> buffer{};
        ::std::span sp{ buffer.begin(), buffer.end() };
        
        ::std::error_code ec;
        
        readable& ref = r;

        ec = co_await ref.read(as_writable_bytes(sp));
        if (ec) co_return false;

        ec = co_await ref.read(as_writable_bytes(sp));
        if (ec) co_return false;

        ec = co_await ref.read(as_writable_bytes(sp));
        if (ec) co_return false;

        co_return ::std::memcmp(buffer.data(), "bcdef", 5) == 0;
    }
}

TEST(readable_test_env, basic)
{
    env_setup().result();
    ASSERT_TRUE(testbody().result());
}
