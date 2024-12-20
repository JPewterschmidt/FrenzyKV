// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <fstream>
#include <filesystem>
#include <print>
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
#include "frenzykv/db/db_local.h"
#include "koios/iouring_awaitables.h"
#include "koios/this_task.h"

#include "MurmurHash3.h"

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;
using namespace ::std::chrono_literals;

namespace r = ::std::ranges;
namespace rv = r::views;
namespace fs = ::std::filesystem;

koios::lazy_task<> db_test(::std::string rootpath = "")
{
    //fs::remove_all(rootpath);

    spdlog::set_pattern("[%H:%M:%S %z] [%^---%L---%$] [thread %t] %v");

    auto opt = get_global_options();
    if (!rootpath.empty())
        opt.root_path = rootpath;
    auto dblocal = co_await db_local::make_unique_db_local("test1", ::std::move(opt));
    db_interface* db = dblocal.get();

    const size_t scale = 100000;
    //const size_t scale = 50000;

    co_await db->init();

    auto t = toolpex::tic();

    //snapshot s = co_await db->get_snapshot();

    auto insertion_func = [scale](db_interface* db) mutable -> koios::task<>
    { 
        spdlog::debug("A insertion_func emitted");
        co_await koios::this_task::sleep_for(1s);

        // #1
        //auto fut_aw = rv::iota(0) | rv::take(scale) | rv::transform([db](int i) { 
        //    return db->insert(::std::to_string(i), "test value abcdefg abcdefg 3").run_and_get_future();
        //}) | r::to<::std::vector>();
        
        // #1
        spdlog::debug("db_test: start insert");
        for (size_t i{}; i < scale; ++i)
        {
            co_await db->insert(::std::to_string(i), "test value abcdefg abcdefg 3");
            //db->insert(::std::to_string(i), "test value abcdefg abcdefg 3").run();
        }

        //for (size_t i{}; i < fut_aw.size(); ++i)
        //{
        //    auto& item = fut_aw[i];
        //    spdlog::debug("waiting {}", i);
        //    co_await item.get_async();
        //    spdlog::debug("{} got", i);
        //}

        spdlog::debug("db_test: insert complete");
    };

    auto fut1 = insertion_func(db).run_and_get_future();
    //auto fut2 = insertion_func(db).run_and_get_future();

    co_await fut1.get_async();
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
        auto opt = co_await db->get(::std::to_string(50));
        if (opt) ::std::println("{}", opt->to_string_debug());
        else ::std::println("not found");
    }

    {
        auto opt = co_await db->get(::std::to_string(1000));
        if (opt) ::std::println("{}", opt->to_string_debug());
        else ::std::println("not found");
    }

    ::std::vector<::std::pair<int, koios::future<::std::optional<kv_entry>>>> futs_aw;
    for (size_t i{}; i < scale; i += 1000)
    {
        futs_aw.emplace_back(i, db->get(::std::to_string(i)).run_and_get_future());
        //futs_aw.emplace_back(i, db->get(::std::to_string(i), { .snap = s }).run_and_get_future());
    }

    for (auto& [i, futaw] : futs_aw)
    {
        auto opt = co_await futaw.get_async();
        if (opt) ::std::println("{}", opt->to_string_debug());
        else ::std::println("key: {} not found", i);
    }

    spdlog::info(toolpex::toc(t));

    spdlog::debug("before dbclose");
    co_await db->close();
    spdlog::debug("after dbclose");

    co_return;
}

int main(int argc, char** argv)
{
    size_t thrs{};
    spdlog::info("addr of the first variable: 0x{:x}", reinterpret_cast<uint64_t>(&thrs));

    if (argc == 1) thrs = 20;
    else thrs = static_cast<size_t>(::atoi(argv[1]));

    koios::runtime_init(thrs);

    auto f1 = db_test("/tmp/frenzykv1").run_and_get_future();
    //auto f2 = db_test("/tmp/frenzykv2").run_and_get_future();

    f1.get();
    //f2.get();
    
    koios::runtime_exit();
}
