#include "frenzykv/statistics.h"
#include <algorithm>

namespace frenzykv 
{

koios::task<size_t> 
statistics::
approx_data_scale() const noexcept
{
    const size_t bl = co_await data_scale_baseline();
    auto lk = co_await m_mutex.acquire_shared();
    co_return ::std::min(bl, m_data_scale);
}

koios::task<system_health> 
statistics::
system_health_state() const noexcept
{
    auto lk = co_await m_mutex.acquire_shared();
    co_return m_health;
}

} // namespace frenzykv
