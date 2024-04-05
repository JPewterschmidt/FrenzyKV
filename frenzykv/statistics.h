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

    /*! \brief A couroutine which reports some useful infomation.
     *  \return the hot data scale (the number of KV pair in main memory) */
    koios::task<size_t> approx_hot_data_scale() const noexcept;

    /*! \brief A couroutine which reports some useful infomation.
     *  \return the hot data size 
     *          (the number of size in bytes all the KV in the main memory occupied) 
     */
    koios::task<size_t> approx_hot_data_size_bytes() const noexcept;
    koios::task<system_health> system_health_state() const noexcept;
    constexpr size_t hot_data_scale_baseline() const noexcept { return 2 << 16; }   

    /*! \brief Increase the hot data scale
     *  \return A pair which 
     *          the first member is the data scale; 
     *          the second member is the number of data bytes in main memory.
     *          Both values are the newest one.
     *
     *  Because of the usage of coroutine mutex, 
     *  this function are also a coroutine.
     */
    koios::task<::std::pair<size_t, size_t>> 
    increase_hot_data_scale(size_t count, size_t size_bytes) noexcept;

private:
    size_t m_data_scale{};
    size_t m_size_bytes{};
    system_health m_health{ system_health::GOOD };

    mutable koios::shared_mutex m_mutex;
};

} // namespace frenzykv

#endif
