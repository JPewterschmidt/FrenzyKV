#ifndef FRENZYKV_DB_SNAPSHOT_H
#define FRENZYKV_DB_SNAPSHOT_H

namespace frenzykv
{

class snapshot
{
public:
    constexpr snapshot() noexcept = default;   
    sequence_number_t sequence_number() const noexcept { return m_seq; }

private:
    sequence_number_t m_seq{};
    // VERSION REF
};

} // namesapce frenzykv

#endif
