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

koios::eager_task<> db_test()
{
    spdlog::set_level(spdlog::level::debug);

    auto dbimpl = ::std::make_unique<db_impl>("test1", get_global_options());
    db_interface* db = dbimpl.get();

    const size_t scale = 50000;

    co_await db->init();

    // #1
    spdlog::debug("db_test: start insert");
    for (size_t i{}; i < scale; ++i)
    {
        auto k = ::std::to_string(i);
        co_await db->insert(k, "test value abcdefg abcdefg");
    }
    spdlog::debug("db_test: insert complete");

    // #2
    //spdlog::debug("db_test: start insert");
    //for (size_t i{}; i < scale; ++i)
    //{
    //    auto k = ::std::to_string(i);
    //    co_await db->insert(k, "test value abcdefg abcdefg");
    //}
    //spdlog::debug("db_test: insert complete");

    //spdlog::debug("db_test: start remove");
    //for (size_t i{}; i < scale; ++i)
    //{
    //    auto k = ::std::to_string(i);
    //    co_await db->remove_from_db(k);
    //}
    //spdlog::debug("db_test: remove complete");

    {
        auto k = ::std::to_string(50);
        auto opt = co_await db->get(k);
        if (opt) ::std::cout << opt->to_string_debug() << ::std::endl;
        else ::std::cout << "not found" << ::std::endl;
    }

    {
        auto k = ::std::to_string(1000);
        auto opt = co_await db->get(k);
        if (opt) ::std::cout << opt->to_string_debug() << ::std::endl;
        else ::std::cout << "not found" << ::std::endl;
    }

    {
        auto k = ::std::to_string(50);
        co_await db->insert(k, "new value with key equals to 50");

        auto opt = co_await db->get(k);
        if (opt) ::std::cout << opt->to_string_debug() << ::std::endl;
        else ::std::cout << "not found" << ::std::endl;
    }

    for (size_t i{}; i < scale; i += 1000)
    {
        auto k = ::std::to_string(i);
        auto opt = co_await db->get(k);
        if (opt) ::std::cout << opt->to_string_debug() << ::std::endl;
        else ::std::cout << "not found" << ::std::endl;
    }

    spdlog::debug("before dbclose");
    co_await db->close();
    spdlog::debug("after dbclose");

    co_return;
}

int main()
{
    koios::runtime_init(20);

    db_test().result();
    
    koios::runtime_exit();
}
