// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_PARSE_RESULT_H
#define FRENZYKV_PARSE_RESULT_H

namespace frenzykv
{

enum class parse_result_t
{
    success, 
    partial, 
    error,
};

} // namespace frenzykv

#endif
