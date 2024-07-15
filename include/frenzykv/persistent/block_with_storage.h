// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_PERSISTENT_BLOCK_WITH_STORAGE_H
#define FRENZYKV_PERSISTENT_BLOCK_WITH_STORAGE_H

#include "frenzykv/io/inner_buffer.h"

#include "frenzykv/persistent/block.h"

namespace frenzykv
{
    
struct block_with_storage
{
    block_with_storage(block bb, buffer<> sto) noexcept
        : b{ ::std::move(bb) }, s{ ::std::move(sto) }
    {
    }

    block_with_storage(block bb) noexcept
        : b{ ::std::move(bb) }
    {
    }

    block b;

    // Iff s.empty(), then the block uses sstable::s_storage
    buffer<> s;
};

} // namespace frenzykv

#endif
