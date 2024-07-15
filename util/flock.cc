// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "frenzykv/util/flock.h"

namespace frenzykv
{

void proc_flock_handler::notify_hub() noexcept
{
    if (!m_async_hub) return;
    m_async_hub->may_wake_next();
}

void proc_flock_aw_base::await_suspend(koios::task_on_the_fly t) noexcept 
{ 
    m_parent.add_awaiting(::std::move(t)); 
}

bool shared_proc_flock_aw::await_ready()
{
    m_result = try_flock_shared(m_parent.fd());
    return !!m_result;
}

proc_flock_handler shared_proc_flock_aw::await_resume()
{
    auto result = m_result.has_value() 
        ? ::std::move(m_result.value()) : try_flock_shared(m_parent.fd()).value();
    result.set_hub(m_parent);
    return result;
}

bool unique_proc_flock_aw::await_ready()
{
    m_result = try_flock_unique(m_parent.fd());
    return !!m_result;
}

proc_flock_handler unique_proc_flock_aw::await_resume()
{
    auto result = m_result.has_value() 
        ? ::std::move(m_result.value()) : try_flock_unique(m_parent.fd()).value();
    result.set_hub(m_parent);
    return result;
}

} // namespace frenzykv
