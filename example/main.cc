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
    auto dbimpl = ::std::make_unique<db_impl>("test1", get_global_options());
    db_interface* db = dbimpl.get();
    
    read_options opt{ 
        .snap = co_await db->get_snapshot()
    };
    co_await db->remove_from_db("hello");

    auto entry = co_await db->get("hello", opt);
    if (entry) ::std::cout << entry->to_string_debug() << ::std::endl;
    else ::std::cout << "no value" << ::std::endl;

    co_await db->close();

    co_return;
}

int main()
{
    koios::runtime_init(11);

    db_test().result();
    
    koios::runtime_exit();
}
