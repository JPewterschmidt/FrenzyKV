#include "gtest/gtest.h"
#include "frenzykv/io/readable.h"
#include "frenzykv/io/writable.h"
#include "frenzykv/io/in_mem_rw.h"
#include "frenzykv/io/iouring_writable.h"
#include "frenzykv/kvdb_deps.h"
#include "koios/iouring_awaitables.h"


using namespace frenzykv;
using namespace ::std::string_view_literals;

