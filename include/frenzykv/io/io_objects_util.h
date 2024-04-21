#ifndef FRENZYKV_IO_OBJECTS_UTIL_H
#define FRENZYKV_IO_OBJECTS_UTIL_H

#include <memory>

#include "koios/task.h"

#include "frenzykv/io/readable.h"
#include "frenzykv/io/in_mem_rw.h"

namespace frenzykv
{

koios::task<::std::unique_ptr<in_mem_rw>>
disk_to_mem(::std::unique_ptr<random_readable> file);

} // namespace frenzykv

#endif
