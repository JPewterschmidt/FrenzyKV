#include <iostream>
#include "nlohmann/json.hpp"

#include "frenzykv/readable.h"
#include "frenzykv/writable.h"
#include "frenzykv/in_mem_rw.h"

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;

#include "frenzykv/readable.h"
#include "frenzykv/writable.h"
#include "frenzykv/in_mem_rw.h"
#include "frenzykv/posix_writable.h"
#include "koios/iouring_awaitables.h"

#include <fstream>

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;

namespace
{
    in_mem_rw r{3};
    ::std::string filename = "testfile.txt";
    
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
        
        readable& ref = r;

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
        co_await uring::unlink(filename);

        posix_writable w{filename};
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
                if (dummy != test_txt) co_return false;
            }
        }
        co_return true;
    }
}

int main()
try
{
    runtime_init(10);

    env_setup().result();
    ::std::cout << ::std::boolalpha << testbody_posix().result() << ::std::endl;

    return runtime_exit();
}
catch (const ::std::exception& e)
{
    ::std::cerr << e.what() << ::std::endl;
    return runtime_exit();
}
