#include "frenzykv/db/version.h"
#include "koios/coroutine_shared_mutex.h"

namespace frenzykv::version_detials
{

class version_impl_simple : public abstract_versions
{
public:
    virtual koios::task<sequence_number_t> 
    set_last_sequence_number(sequence_number_t newver) override
    {
        auto lk = co_await m_mut.acquire();
        co_return (m_last_seq = newver);
    }

    virtual koios::task<sequence_number_t> 
    last_sequence_number() const override
    { 
        auto lk = co_await m_mut.acquire_shared();
        co_return m_last_seq;
    }

private:
    mutable koios::shared_mutex m_mut;
    sequence_number_t m_last_seq{};
};

::std::unique_ptr<abstract_versions> 
default_instance(::std::string filename)
{
    using namespace version_detials;
    return ::std::make_unique<version_impl_simple>();
}

} // namespace frenzykv::version_detials

namespace frenzykv
{

version_hub::version_hub(const kvdb_deps& deps)
    : m_deps{ &deps }, 
      m_pimpl{ version_detials::default_instance() }
{
    assert(m_pimpl != nullptr);
    assert(m_deps != nullptr);
}

koios::task<> 
version_hub::
set_last_sequence_number(sequence_number_t newver)
{
    co_await m_pimpl->set_last_sequence_number(::std::move(newver));
}

koios::task<sequence_number_t>
version_hub::
last_sequence_number() const 
{ 
    co_return co_await m_pimpl->last_sequence_number(); 
}

} // namespace frenzykv
