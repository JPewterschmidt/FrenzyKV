#include <iostream>
#include "nlohmann/json.hpp"

#include "frenzykv/readable.h"
#include "frenzykv/writable.h"
#include "frenzykv/in_mem_rw.h"
#include "frenzykv/iouring_writable.h"
#include "frenzykv/iouring_readable.h"
#include "frenzykv/util/multi_dest_record_writer.h"
#include "frenzykv/util/stdout_debug_record_writer.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/util/key_cmp.h"
#include "frenzykv/db/db_impl.h"
#include "koios/iouring_awaitables.h"
#include "entry_pbrep.pb.h"

#include "MurmurHash3.h"

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
    size_t hash_result{};
    seq_key k;
    k.set_user_key("xxxxxxxxx");
    k.set_seq_number(0);

    ::std::array<char, 128> buffer{};
    k.SerializeToArray(buffer.data(), buffer.size());
}
