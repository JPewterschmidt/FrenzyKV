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
#include "db/db_impl.h"
#include "util/key_cmp.h"

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;

task<> ostest()
try
{
    seq_key key1, key2, key3, key4;
    ::std::string key1_str, key2_str, key3_str, key4_str;

    key1.set_user_key("xxxxxxxxx");
    key1.set_seq_number(0);

    key2.set_user_key("xxxxxxxxx");
    key2.set_seq_number(1);

    key3.set_user_key("xxxxxxxxx");
    key3.set_seq_number(10000000);

    key4.set_user_key("xxxxxxxxx");
    key4.set_seq_number(100000000);

    key1.SerializeToString(&key1_str);
    key2.SerializeToString(&key2_str);
    key3.SerializeToString(&key3_str);
    key4.SerializeToString(&key4_str);

    ::std::cout << ::std::less{}(key1_str, key2_str) << ::std::endl;
    ::std::cout << ::std::less{}(key1_str, key3_str) << ::std::endl;
    ::std::cout << ::std::less{}(key3_str, key4_str) << ::std::endl;

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
