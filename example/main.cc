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

koios::eager_task<> file_test()
{
    ::std::unique_ptr<kvdb_deps> deps = ::std::make_unique<kvdb_deps>();
    auto file = deps->env()->get_seq_writable("test");

    co_await file->append("123");
    co_await file->close();
    
    auto file2 = deps->env()->get_seq_readable("test");
    ::std::string buffer(53, 0);
    ::std::span<char> bs{ buffer.data(), buffer.size() };
    
    size_t readed = co_await file2->read(::std::as_writable_bytes(bs));
    ::std::cout << readed << ::std::endl;

    co_await uring::unlink("test");

    co_return;
}

int main()
{
    //auto opt = get_global_options();
    //nlohmann::json j(opt);
    //::std::ofstream ofs{ "test-config.json" };
    //ofs << j.dump(4);
    //::std::cout << j.dump(4) << ::std::endl;

    koios::runtime_init(11);

    file_test().result();
    
    koios::runtime_exit();
}
