#include <iostream>
#include "nlohmann/json.hpp"

#include "frenzykv/readable.h"
#include "frenzykv/writable.h"
#include "frenzykv/in_mem_rw.h"

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;

int main()
try
{
    runtime_init(10);

    return runtime_exit();
}
catch (const ::std::exception& e)
{
    ::std::cerr << e.what() << ::std::endl;
    return runtime_exit();
}
