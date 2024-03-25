#include "db/version.h"

namespace frenzykv
{

version_hub::version_hub(const options& opt)
    : m_opt{ &opt } // TODO pimpl
{
}

void 
version_hub::
commit(version_record newver)
{
    assert(m_pimpl);
    m_pimpl->commit(::std::move(newver));
}

version_record 
version_hub::
current() const
{
    assert(m_pimpl);
    return m_pimpl->current();
}

sequence_number_t 
version_hub::
last_sequence_number() const 
{ 
    return m_pimpl->last_sequence_number(); 
}

} // namespace frenzykv
