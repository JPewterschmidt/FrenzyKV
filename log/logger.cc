#include <format>
#include <cassert>
#include <filesystem>

#include "toolpex/functional.h"

#include "koios/iouring_awaitables.h"

#include "frenzykv/error_category.h"

#include "frenzykv/log/logger.h"

#include "frenzykv/io/inner_buffer.h"

namespace fs = ::std::filesystem;

namespace frenzykv
{

::std::string prewrite_log_name()
{
    return "log.frzkvlog";
}

koios::task<> logger::insert(const write_batch& b)
{
    co_await koios::this_task::turn_into_scheduler();
    auto lk = co_await m_mutex.acquire();

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
    co_await may_flush_impl();
}

koios::task<> logger::truncate_file() noexcept
{
    auto lk = co_await m_mutex.acquire();
    m_log_file = m_deps->env()->get_truncate_seq_writable(prewrite_log_path()/prewrite_log_name());
    assert(!!m_log_file);
    co_return;
}

koios::task<bool> logger::empty() const noexcept
{
    auto lk = co_await m_mutex.acquire();
    co_return m_log_file->file_size() == 0;
}

koios::task<> logger::may_flush(bool force)
{
    auto lk = co_await m_mutex.acquire();
    co_await may_flush_impl(force);
}

koios::task<> logger::may_flush_impl(bool force) 
{
    // If the data scale is not that big, just flush easy to debug.
    if (const auto s = m_deps->stat(); 
        force || !s || (s && co_await s->approx_hot_data_scale() <= s->hot_data_scale_baseline()))
    {
        co_await m_log_file->flush();
    }
}

koios::task<::std::pair<write_batch, sequence_number_t>> 
recover(env* e) noexcept
{
    ::std::error_code ec;
    if (uintmax_t sz = fs::file_size(prewrite_log_path()/prewrite_log_name(), ec);
        ec || sz == 0)
    {
        co_return {};
    }

    auto filep = e->get_seq_readable(prewrite_log_path()/prewrite_log_name());
    assert(!!filep);
    const uintmax_t filesz = filep->file_size();
    buffer<> buf(filesz);
    const size_t readed = co_await filep->read(buf.writable_span());
    assert(readed == filesz);
    buf.commit(readed);

    sequence_number_t max_seq{};
    write_batch result;
    for (auto kv : kv_entries_from_buffer(buf.valid_span()))
    {
        if (const auto cur_seq = kv.key().sequence_number();
            cur_seq > max_seq)
        {
            max_seq = cur_seq;
        }
        result.write(::std::move(kv));
    }
    
    co_return { result, max_seq };
}

koios::eager_task<> logger::delete_file()
{
    auto lk = co_await m_mutex.acquire();
    co_await koios::uring::unlink(prewrite_log_path()/prewrite_log_name());
}

} // namespace frenzykv
