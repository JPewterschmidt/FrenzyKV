#include <iostream>
#include "nlohmann/json.hpp"

#include "frenzykv/readable.h"
#include "frenzykv/writable.h"
#include "frenzykv/in_mem_rw.h"
#include "frenzykv/iouring_writable.h"
#include "frenzykv/iouring_readable.h"

#include "util/multi_dest_record_writer.h"
#include "util/stdout_debug_record_writer.h"

#include "koios/iouring_awaitables.h"
#include "frenzykv/write_batch.h"

#include "entry_pbrep.pb.h"

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;

task<> ostest()
try
{
    write_batch batch;
    batch.write("xxx", "123");
    batch.write("xxx", "123");
    batch.write("xxx", "345");
    batch.write("xxx", "345");
    batch.write("xxx", "345");
    batch.write("xxx", "345");

    write_batch batch2 = batch;
    batch2.write("yyy", "123");
    batch2.write("yyy", "123");
    batch2.write("yyy", "123");
    batch2.write("yyy", "123");
    batch2.remove_from_db("yyy");

    
    record_writer_wrapper writer = multi_dest_record_writer{}
        .new_with(stdout_debug_record_writer{})
        .new_with(stdout_debug_record_writer{})
        .new_with(stdout_debug_record_writer{});

    co_await writer.write(batch2);
    co_await writer.write(batch);
    
    co_return;
}
catch (const koios::exception& e)
{
    e.log();
}

int main()
{
    koios::runtime_init(4);
    ostest().result();
    koios::get_task_scheduler().stop();
    return koios::runtime_exit();
}
