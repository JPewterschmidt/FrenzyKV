// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"

#include "toolpex/tic_toc.h"

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
#include "koios/this_task.h"

#include "MurmurHash3.h"

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;
using namespace ::std::chrono_literals;

koios::eager_task<> db_test()
{
    spdlog::set_level(spdlog::level::debug);

    auto dbimpl = ::std::make_unique<db_impl>("test1", get_global_options());
    db_interface* db = dbimpl.get();

    const size_t scale = 100000;
    //const size_t scale = 50000;

    co_await db->init();

    auto t = toolpex::tic();

    snapshot s = co_await db->get_snapshot();

    auto insertion_func = [](db_interface* db) mutable -> koios::task<>
    { 
        spdlog::debug("A insertion_func emitted");
        co_await koios::this_task::sleep_for(1s);
        
        // #1
        spdlog::debug("db_test: start insert");
        for (size_t i{}; i < scale; ++i)
        {
            auto k = ::std::to_string(i);
            co_await db->insert(k, "test value abcdefg abcdefg 3");
        }
        spdlog::debug("db_test: insert complete");
    };

    //auto fut1 = insertion_func(db).run_and_get_future();
    //auto fut2 = insertion_func(db).run_and_get_future();

    //co_await fut1.get_async();
    //co_await fut2.get_async();

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

    for (size_t i{}; i < scale; i += 1000)
    {
        auto k = ::std::to_string(i);
        auto opt = co_await db->get(k, { .snap = s });
        if (opt) ::std::cout << opt->to_string_debug() << ::std::endl;
        else ::std::cout << "not found" << ::std::endl;
    }

    spdlog::info(toolpex::toc(t));

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
