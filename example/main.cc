#include <iostream>
#include "nlohmann/json.hpp"

#include "frenzykv/readable.h"
#include "frenzykv/writable.h"
#include "frenzykv/in_mem_rw.h"
#include "frenzykv/iouring_writable.h"
#include "frenzykv/iouring_readable.h"
#include "koios/iouring_awaitables.h"

#include "entry_pbrep.pb.h"

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;

task<> ostest()
try
{
    entry_pbrep obj1;
    obj1.set_key(reinterpret_cast<const ::std::byte*>("123"sv.data()), 3);
    obj1.set_value(reinterpret_cast<const ::std::byte*>("abc"sv.data()), 3);

    entry_pbrep obj2;
    ::std::string out1;
    obj1.SerializeToString(&out1);
    obj2.set_key(out1);
    obj2.set_value(out1);

    iouring_readable f{ "testtesttest" };
    buffer<> buf{};
    buf.commit(co_await f.read(buf.writable_span()));
    entry_pbrep obj3;
    obj3.ParseFromArray(buf.valid_span().data(), (int)buf.valid_span().size());

    ::std::cout << obj3.DebugString() << ::std::endl;

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
