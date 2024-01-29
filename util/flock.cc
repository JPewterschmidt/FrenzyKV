#include "util/flock.h"

namespace frenzykv
{

void flock_handler::notify_hub() noexcept
{
    if (!m_async_hub) return;
    m_async_hub->may_wake_next();
}

void flock_aw_base::await_suspend(koios::task_on_the_fly t) noexcept 
{ 
    m_parent.add_awaiting(::std::move(t)); 
}

bool shared_flock_aw::await_ready()
{
    m_result = try_flock_shared(m_parent.fd());
    return !!m_result;
}

flock_handler shared_flock_aw::await_resume()
{
    auto result = m_result.has_value() 
        ? ::std::move(m_result.value()) : try_flock_shared(m_parent.fd()).value();
    result.set_hub(m_parent);
    return result;
}

bool unique_flock_aw::await_ready()
{
    m_result = try_flock_unique(m_parent.fd());
    return !!m_result;
}

flock_handler unique_flock_aw::await_resume()
{
    auto result = m_result.has_value() 
        ? ::std::move(m_result.value()) : try_flock_unique(m_parent.fd()).value();
    result.set_hub(m_parent);
    return result;
}

} // namespace frenzykv
