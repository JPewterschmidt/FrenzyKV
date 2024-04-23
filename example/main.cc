#include <iostream>
#include <ranges>
#include <fstream>
#include <vector>
#include <iterator>
#include "nlohmann/json.hpp"

#include "frenzykv/io/readable.h"
#include "frenzykv/io/writable.h"
#include "frenzykv/io/in_mem_rw.h"
#include "frenzykv/io/iouring_writable.h"
#include "frenzykv/io/iouring_readable.h"
#include "frenzykv/util/multi_dest_record_writer.h"
#include "frenzykv/util/stdout_debug_record_writer.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/db/db_impl.h"
#include "koios/iouring_awaitables.h"

#include "MurmurHash3.h"

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;
namespace rv = ::std::ranges::views;

koios::task<> file_test()
{
    co_return;
}

int main()
{
    const ::std::string user_value = "WilsonAlinaWilsonAlinaWilsonAlinaWilsonAlina";
    const ::std::string tomb_stone_key = "bbbb test";
    ::std::vector<kv_entry> kvs{};
    ::std::string key = "aaabbbccc";

    kvs.emplace_back(0, tomb_stone_key);
    for (size_t i{}; i < 1000; ++i)
    {
        if (i % 20 == 0)
        {
            auto newkview = key | rv::transform([](auto&& ch){ return ch + 1; });
            key = ::std::string{ begin(newkview), end(newkview) };
        }

        kvs.emplace_back((sequence_number_t)i, key, user_value);
    }
    ::std::sort(kvs.begin(), kvs.end());
}
