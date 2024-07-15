// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "frenzykv/db/snapshot.h"

namespace frenzykv
{

void snapshot_center::
set_init_leatest_used_sequence_number(sequence_number_t seq) noexcept
{
    sequence_number_t cur_seq = leatest_used_sequence_number();
    if (cur_seq >= seq + 1) return;
    m_newest_unused_seq.store(seq + 1);
}

} // namespace frenzykv
