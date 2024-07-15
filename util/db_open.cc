// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "frenzykv/db.h"

namespace frenzykv
{

::std::unique_ptr<db_interface> open(toolpex::ip_address::ptr ip, in_port_t port)
{
    return nullptr;
}

::std::unique_ptr<db_interface> open(::std::filesystem::path path, options opt)
{
    return nullptr;
}

} // namespace frenzykv
