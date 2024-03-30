#ifndef FRENZYKV_STATISTICS_H
#define FRENZYKV_STATISTICS_H

#include <utility>
#include "koios/task.h"
#include "koios/coroutine_shared_mutex.h"

namespace frenzykv
{

enum class system_health : unsigned 
{
    GOOD = 0, ERROR, DEAD,
};

class statistics
{
public:
    friend class statistics_updater;
    koios::task<size_t> approx_hot_data_scale() const noexcept;
    koios::task<size_t> approx_hot_data_size_bytes() const noexcept;
    koios::task<system_health> system_health_state() const noexcept;
    constexpr size_t hot_data_scale_baseline() const noexcept { return 2 << 16; }   

    koios::task<::std::pair<size_t, size_t>> 
    increase_hot_data_scale(size_t count, size_t size_bytes) noexcept;

private:
    size_t m_data_scale{};
    size_t m_size_bytes{};
    system_health m_health{ system_health::GOOD };

    mutable koios::shared_mutex m_mutex;
};

statistics& global_statistics() noexcept;

} // namespace frenzykv

#endif
