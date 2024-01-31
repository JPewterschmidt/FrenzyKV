#include <iostream>
#include "nlohmann/json.hpp"

#include "frenzykv/readable.h"
#include "frenzykv/writable.h"
#include "frenzykv/in_mem_rw.h"
#include "frenzykv/iouring_writable.h"
#include "koios/iouring_awaitables.h"

#include "entry_pbrep.pb.h"

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;

task<> ostest()
{
    entry_pbrep obj1;
    obj1.set_key(reinterpret_cast<const ::std::byte*>("123"sv.data()), 3);
    obj1.set_value(reinterpret_cast<const ::std::byte*>("abc"sv.data()), 3);

    entry_pbrep obj2;
    ::std::string out1;
    obj1.SerializeToString(&out1);
    obj2.set_key(out1);
    obj2.set_value(out1);

    co_await uring::unlink("testtesttest.txt");
    iouring_writable w{ "testtesttest.txt" };
    ::std::ostream wos{ w.streambuf() };
    obj2.SerializeToOstream(&wos);

    co_await w.sync();
    ::std::cout << "ok" << ::std::endl;

    co_return;
}

int main()
{
    koios::runtime_init(4);
    ostest().result();
    koios::get_task_scheduler().stop();
    return koios::runtime_exit();
}
