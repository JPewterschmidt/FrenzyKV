#include <iostream>
#include "nlohmann/json.hpp"

#include "frenzykv/readable.h"
#include "frenzykv/writable.h"
#include "frenzykv/in_mem_rw.h"

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;

namespace
{
    in_mem_rw r{6};
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

int main()
{
    runtime_init(10);

    env_setup().result();
    ::std::cout << testbody().result() << ::std::endl;

    runtime_exit();
}
