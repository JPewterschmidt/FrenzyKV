// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_READ_WRITE_OPTIONS_H
#define FRENZYKV_READ_WRITE_OPTIONS_H

#include "frenzykv/db/snapshot.h"

namespace frenzykv
{

struct write_options
{
    bool sync_write = false;
};

struct read_options
{
    snapshot snap{};
};

} // namespace frenzykv

#endif
