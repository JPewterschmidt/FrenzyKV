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
approx_hot_data_scale() const noexcept
{
    const size_t bl = hot_data_scale_baseline();
    auto lk = co_await m_mutex.acquire_shared();
    co_return ::std::max(bl, m_data_scale);
}

koios::task<size_t> 
statistics::
approx_hot_data_size_bytes() const noexcept
{
    auto lk = co_await m_mutex.acquire_shared();
    co_return m_size_bytes;
}

koios::task<system_health> 
statistics::
system_health_state() const noexcept
{
    auto lk = co_await m_mutex.acquire_shared();
    co_return m_health;
}

koios::task<::std::pair<size_t, size_t>> 
statistics::
increase_hot_data_scale(size_t count, size_t size_bytes) noexcept
{
    auto lk = co_await m_mutex.acquire();
    m_data_scale += count;
    m_size_bytes += size_bytes;
    
    co_return { m_data_scale, m_size_bytes };
}

} // namespace frenzykv
