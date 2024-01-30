#include <iostream>
#include "nlohmann/json.hpp"

#include "frenzykv/readable.h"
#include "frenzykv/writable.h"
#include "frenzykv/in_mem_rw.h"

#include "entry_pbrep.pb.h"

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;

int main()
{
    entry_pbrep obj1;
    obj1.set_key(reinterpret_cast<const ::std::byte*>("123"sv.data()), 3);
    obj1.set_value(reinterpret_cast<const ::std::byte*>("abc"sv.data()), 3);

    entry_pbrep obj2;
    ::std::string out1;
    obj1.SerializeToString(&out1);
    obj2.set_key(out1);
    obj2.set_value(out1);

    ::std::string out2;
    obj2.SerializeToString(&out2);

    ::std::cout << ::std::hex << obj2.DebugString() << ::std::endl;
    ::std::cout << ::std::hex << obj1.DebugString() << ::std::endl;
}
