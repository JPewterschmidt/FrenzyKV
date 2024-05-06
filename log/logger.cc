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

koios::task<> logger::truncate_file() noexcept
{
    m_log_file = m_deps->env()->get_truncate_seq_writable(prewrite_log_path()/prewrite_log_name());
    assert(!!m_log_file);
    co_return;
}

koios::task<bool> logger::empty() const noexcept
{
    co_return m_log_file->file_size() == 0;
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

koios::task<write_batch> recover(env* e) noexcept
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

    write_batch result;
    for (auto kv : kv_entries_from_buffer(buf.valid_span()))
    {
        result.write(::std::move(kv));
    }
    
    co_return result;
}

koios::eager_task<> logger::delete_file()
{
    co_await koios::uring::unlink(prewrite_log_path()/prewrite_log_name());
}

} // namespace frenzykv
