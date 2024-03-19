#include "frenzykv/statistics.h"
#include <algorithm>
#include <utility>

namespace frenzykv 
{

static statistics& global_statistics_impl() noexcept
{
    static statistics result{};
    return result;
}

statistics& global_statistics() noexcept
{
    return (global_statistics_impl());
}

koios::task<size_t> 
statistics::
approx_data_scale() const noexcept
{
    const size_t bl = data_scale_baseline();
    auto lk = co_await m_mutex.acquire_shared();
    co_return ::std::max(bl, m_data_scale);
}

koios::task<system_health> 
statistics::
system_health_state() const noexcept
{
    auto lk = co_await m_mutex.acquire_shared();
    co_return m_health;
}

} // namespace frenzykv
