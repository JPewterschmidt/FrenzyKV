#ifndef FRENZYKV_STATISTICS_H
#define FRENZYKV_STATISTICS_H

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
    koios::task<size_t> approx_data_scale() const noexcept;
    koios::task<system_health> system_health_state() const noexcept;
    koios::task<size_t> data_scale_baseline() const noexcept { co_return 2 << 16; }   

private:
    size_t m_data_scale{};
    system_health m_health{ system_health::GOOD };

    mutable koios::shared_mutex m_mutex;
};

const statistics& global_statistics() noexcept;

} // namespace frenzykv

#endif
