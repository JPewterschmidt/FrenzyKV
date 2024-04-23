#include "frenzykv/log/logger.h"
#include "frenzykv/error_category.h"
#include "toolpex/functional.h"
#include <format>

namespace frenzykv
{

koios::task<> logger::insert(const write_batch& b)
{
    co_await koios::this_task::turn_into_scheduler();

    for (const auto& item : b)
    {
        const size_t item_size = item.serialized_bytes_size();
        auto writable = m_log_file->writable_span();
        if (writable.size_bytes() < item_size)
        {
            co_await m_log_file->flush();
            writable = m_log_file->writable_span();
        }
        if (item.serialize_to(writable))
        {
            co_await m_log_file->commit(item_size);
        }
        else [[unlikely]]
        {
            throw koios::exception(::std::error_code(FRZ_KVDB_SERIZLIZATION_ERR, kvdb_category()));
        }
    }
    co_await may_flush();
}

koios::task<> logger::may_flush(bool force) noexcept
{
    // If the data scale is not that big, just flush easy to debug.
    if (const auto s = m_deps->stat(); 
        force || !s || (s && co_await s->approx_hot_data_scale() <= s->hot_data_scale_baseline()))
    {
        co_await m_log_file->flush();
    }
}

} // namespace frenzykv
