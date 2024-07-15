// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_IO_OBJECTS_UTIL_H
#define FRENZYKV_IO_OBJECTS_UTIL_H

#include <memory>

#include "koios/task.h"

#include "frenzykv/io/readable.h"
#include "frenzykv/io/in_mem_rw.h"

namespace frenzykv
{

koios::task<in_mem_rw> to_in_mem_rw(random_readable& file);

} // namespace frenzykv

#endif
