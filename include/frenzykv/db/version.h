#ifndef FRENZYKV_DB_VERSION_H
#define FRENZYKV_DB_VERSION_H

namespace frenzykv
{

class version_set
{
public:
    version_set(const kvdb_deps& deps, sequence_number_t current)
        : m_deps{ &deps }, m_current{ current };
    {
    }

private:
    const kvdb_deps* m_deps;
    ::std::set<sequence_number_t> m_currently_using;
    sequence_number_t m_current;
};

} // namespace frenzykv

#endif
