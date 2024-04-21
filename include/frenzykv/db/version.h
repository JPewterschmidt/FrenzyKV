#ifndef FRENZYKV_DB_VERSION_H
#define FRENZYKV_DB_VERSION_H

#include <memory>

#include "toolpex/ref_count.h"
#include "koios/task.h"

namespace frenzykv
{

class version_delta
{
public:
private:
};

class version
{
public:
    using sptr = ::std::shared_ptr<version>;
    
public:
    sequence_number_t sequence_number() const noexcept { return m_seq; }
    ~version() noexcept;

    bool    operator<(const version& other) const noexcept;
    version operator+(const version_delta& delta) const noexcept;

private:
    sequence_number_t m_seq;
};

class version_set
{
public:
    version_set(const kvdb_deps& deps, sequence_number_t current)
        : m_deps{ &deps }, m_current{ current };
    {
    }
    
private:
    const kvdb_deps* m_deps;
    sequence_number_t m_current;
};

} // namespace frenzykv

#endif
