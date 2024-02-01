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
    entry_pbrep obj;
    obj.set_key("xxx", 1);
    obj.set_value("y", 1);
    ::std::cout << obj.DebugString() << ::std::endl;
    ::std::cout << obj.ByteSizeLong() << ::std::endl;
    
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
