#include <iostream>
#include <fstream>
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

koios::task<> db_test()
{
    spdlog::set_level(spdlog::level::debug);

    auto dbimpl = ::std::make_unique<db_impl>("test1", get_global_options());
    db_interface* db = dbimpl.get();

    const size_t scale = 10000;

    spdlog::debug("start insert");
    for (size_t i{}; i < scale; ++i)
    {
        auto k = ::std::to_string(i);
        co_await db->insert(k, "testtest");
    }
    spdlog::debug("insert complete");

    auto k = ::std::to_string(50);
    auto opt = co_await db->get(k);
    if (opt)
    {
        ::std::cout << opt->to_string_debug() << ::std::endl;
    }
    else ::std::cout << "not found" << ::std::endl;

    co_await dbimpl->compact_tombstones();

    spdlog::debug("before dbclose");
    co_await db->close();
    spdlog::debug("after dbclose");

    co_return;
}

int main()
{
    koios::runtime_init(10);

    db_test().result();
    
    koios::runtime_exit();
}
