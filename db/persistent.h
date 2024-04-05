#ifndef FRENZYKV_PERSISTENT_H
#define FRENZYKV_PERSISTENT_H

#include <cassert>
#include <memory>
#include "entry_pbrep.pb.h"
#include "frenzykv/frenzykv.h"
#include "frenzykv/kvdb_deps.h"
#include "db/memtable.h"

namespace frenzykv
{

template<typename PersisImpl>
class persistent
{
public:
    persistent(const kvdb_deps& deps)
        : m_pimpl{ ::std::make_unique<PersisImpl>(deps) }
    {
        assert(m_pimpl);
    }

    /*! \brief  Search KV from the persistent storage.
     *  \return Return a awaitable object which result type is `::std::optional<entry_pbrep>`
     *          Probabely `koios::task<::std::optional<entry_pbrep>>`, 
     *          based on the template parameter `PersisImpl` 
     *          of the class this member function belongs to.
     *
     *  The implementation is actually forwarding this get requests 
     *  to the object pointed by pimpl which type is the 
     *  template argument you specified `PersisImpl`.
     *
     *  \see `persistent` 
     */
    auto get(const seq_key& k) const
    {
        return m_pimpl->get(k);
    }

    /*! \brief  Write a whole memtable to the persistent storage.
     *  \return Return a awaitable object, Probabely `koios::task<>`.
     *          based on the template parameter `PersisImpl` 
     *          of the class this member function belongs to.
     *
     *  The implementation is actually forwarding this get requests 
     *  to the object pointed by pimpl which type is the 
     *  template argument you specified `PersisImpl`.
     *
     *  \see `persistent` 
     */
    koios::task<> write(imm_memtable table)
    {
        return m_pimpl->write(::std::move(table));       
    }

private:
    ::std::unique_ptr<PersisImpl> m_pimpl;
};

} // namespace frenzykv

#endif
