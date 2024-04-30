#include "frenzykv/db/snapshot.h"

namespace frenzykv
{

void snapshot_center::
set_newest_sequence_number(sequence_number_t seq) noexcept
{
    sequence_number_t prev = seq - 1;
    while (!m_newest_seq.compare_exchange_weak(
                prev, seq, 
                ::std::memory_order_release, 
                ::std::memory_order_relaxed))
    {
        assert(prev < seq);
    }
}

} // namespace frenzykv
