// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_LOGGING_LEVEL_H
#define FRENZYKV_LOGGING_LEVEL_H

#include <cstddef>
#include <string>
#include <string_view>

namespace frenzykv
{

enum class logging_level : ::std::size_t
{
    DEBUG = 0, 
    INFO  = 1, 
    ERROR = 2, 
    ALERT = 3, 
    FATAL = 4,
};

::std::string_view to_string(logging_level l) noexcept;
logging_level to_logging_level(::std::string_view lstr);

} // namespace frenzykv

#endif
