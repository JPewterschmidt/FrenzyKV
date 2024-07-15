// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_CONCEPTS_H
#define FRENZYKV_CONCEPTS_H

#include "koios/task_concepts.h"

namespace frenzykv
{

template<typename PersisImpl>
concept persistent_concept = requires (PersisImpl p)
{
    { p.get(::std::declval<seq_key>()) }        -> awaitible_concept;
    { p.write(::std::declval<imm_memtable>() }  -> awaitible_concept;
};

} // namespace frenzykv

#endif
