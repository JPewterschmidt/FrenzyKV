#include "log/logger.h"
#include "frenzykv/error_category.h"
#include "toolpex/functional.h"
#include <format>

namespace frenzykv
{

::std::string logger::to_string() const
{
    return ::std::format("logger: name = {}, path = {}", 
        filename(), filepath().string());
}

koios::task<> logger::write(const write_batch& b)
{
    co_await koios::this_task::turn_into_scheduler();

    for (const auto& item : b)
    {
        const size_t item_size = item.ByteSizeLong();
        auto writable = m_log_file->writable_span();
        if (writable.size_bytes() < item_size)
        {
            co_await m_log_file->flush();
            writable = m_log_file->writable_span();
        }
        if (item.SerializeToArray(writable.data(), static_cast<int>(writable.size_bytes())))
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
    if (const auto* s = m_opt->stat; 
        force || !s || (s && co_await s->approx_hot_data_scale() < 100))
    {
        co_await m_log_file->flush();
    }
}

} // namespace frenzykv
