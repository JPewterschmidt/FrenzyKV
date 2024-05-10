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

koios::task<> parallel_insert(db_interface* db)
{
    for (size_t i{}; i < 50000; ++i)
    {
        co_await db->insert(::std::to_string(i), "testtest");
    }
}

koios::task<> db_test()
{
    auto dbimpl = ::std::make_unique<db_impl>("test1", get_global_options());
    db_interface* db = dbimpl.get();

    auto f1 = parallel_insert(db).run_and_get_future();
    auto f2 = parallel_insert(db).run_and_get_future();
    
    f1.get();
    f2.get();

    auto k = ::std::to_string(50);
    auto opt = co_await db->get(k);
    if (opt)
    {
        ::std::cout << opt->to_string_debug() << ::std::endl;
    }
    else ::std::cout << "not found" << ::std::endl;

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
