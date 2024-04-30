#ifndef FRENZYKV_DB_SNAPSHOT_H
#define FRENZYKV_DB_SNAPSHOT_H

#include <atomic>

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

private:
    sequence_number_t m_seq{};
    version_guard m_version;
};

class snapshot_center
{
public:
    sequence_number_t newest_sequence_number() const noexcept 
    { 
        return m_newest_seq.load(); 
    }   

    void set_newest_sequence_number(sequence_number_t seq) noexcept;

    snapshot get_snapshot(version_guard version) noexcept
    {
        return { newest_sequence_number(), ::std::move(version) };
    }
       
private:
    ::std::atomic<sequence_number_t> m_newest_seq;
};

} // namesapce frenzykv

#endif
