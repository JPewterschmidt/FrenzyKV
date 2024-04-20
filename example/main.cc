#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"

#include "frenzykv/readable.h"
#include "frenzykv/writable.h"
#include "frenzykv/in_mem_rw.h"
#include "frenzykv/iouring_writable.h"
#include "frenzykv/iouring_readable.h"
#include "frenzykv/util/multi_dest_record_writer.h"
#include "frenzykv/util/stdout_debug_record_writer.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/db/db_impl.h"
#include "koios/iouring_awaitables.h"

#include "MurmurHash3.h"

using namespace koios;
using namespace frenzykv;
using namespace ::std::string_view_literals;

koios::task<> file_test()
{
    co_return;
}

int main()
{
    auto opt = get_global_options();
    nlohmann::json j(opt);
    ::std::ofstream ofs{ "test-config.json" };
    ofs << j.dump(4);
    ::std::cout << j << ::std::endl;
}
