#ifndef FRENZYKV_VERSION_H
#define FRENZYKV_VERSION_H

#include <memory>
#include <chrono>
#include "frenzykv/options.h"

namespace frenzykv
{

using sequence_number_t = uint64_t;

struct version_record
{
    sequence_number_t m_sequence_number{};
    ::std::chrono::system_clock::time_point m_timestemp{ ::std::chrono::system_clock::now() };
};

class abstract_versions
{
public:
    virtual version_record current() const = 0;
    virtual void commit(version_record newver) = 0;
    virtual sequence_number_t last_sequence_number() const { return current().m_sequence_number; }
};

class version_hub
{
public:
    version_hub(const options& opt = get_global_options());

    void commit(version_record newver)
    {
        assert(m_pimpl);
        m_pimpl->commit(::std::move(newver));
    }

    version_record current() const
    {
        assert(m_pimpl);
        return m_pimpl->current();
    }

private:
    const options* m_opt;
    const ::std::unique_ptr<abstract_versions> m_pimpl;
};

} // namespace frenzykv

#endif
