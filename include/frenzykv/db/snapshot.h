#ifndef FRENZYKV_DB_SNAPSHOT_H
#define FRENZYKV_DB_SNAPSHOT_H

#include <atomic>
#include <limits>

#include "frenzykv/types.h"
#include "frenzykv/db/version.h"

namespace frenzykv
{

class snapshot
{
public:
    constexpr snapshot() noexcept = default;

    snapshot(sequence_number_t seq, version_guard version) noexcept
        : m_seq{ seq }, m_version{ ::std::move(version) }
    {
    }

    sequence_number_t sequence_number() const noexcept { return m_seq; }
    constexpr bool valid() const noexcept { return m_seq != 0; }

private:
    sequence_number_t m_seq{};
    version_guard m_version;
};

class snapshot_center
{
public:
    snapshot get_snapshot(version_guard version) noexcept
    {
        return { leatest_used_sequence_number(), ::std::move(version) };
    }

    sequence_number_t leatest_used_sequence_number() const noexcept { return m_newest_seq.load(); }

    sequence_number_t get_next_unused_sequence_number(size_t count = 1) noexcept 
    { 
        assert(count < ::std::numeric_limits<sequence_number_t>::max());
        return m_newest_seq.fetch_add(static_cast<sequence_number_t>(count)); 
    }

private:
    ::std::atomic<sequence_number_t> m_newest_seq{1};
};

} // namesapce frenzykv

#endif
