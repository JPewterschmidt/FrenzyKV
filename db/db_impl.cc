#include "db/db_impl.h"
#include "util/multi_dest_record_writer.h"
#include "util/stdout_debug_record_writer.h"
#include "log/logger.h"
#include "frenzykv/statistics.h"
#include "frenzykv/error_category.h"
#include "frenzykv/options.h"
#include <cassert>

namespace frenzykv
{

db_impl::db_impl(::std::string dbname, const options& opt)
    : m_dbname{ ::std::move(dbname) }, 
      m_opt{ opt }, 
      m_env{ env::make_default_env(m_opt) }, 
      m_log{ (m_opt.environment = m_env.get(), m_opt), "0001-test.frzkvlog" }, 
      m_memset{ m_opt }
{
    m_opt.stat = &m_stat;
    assert(m_opt.environment);
    assert(m_opt.stat);
}

db_impl::~db_impl() noexcept
{
    m_stp_src.request_stop();
}

koios::task<size_t> 
db_impl::write(write_batch batch) 
{
    const size_t serialized_size = batch.serialized_size();

    co_await may_prepare_space(batch);
    co_await m_log.write(batch);
    co_await m_memset.insert(::std::move(batch));
    auto [total_count, _] = co_await m_stat.increase_hot_data_scale(batch.count(), serialized_size);
    (void)_;

    co_return total_count;
}

koios::task<::std::optional<entry_pbrep>> 
db_impl::get(const_bspan key, ::std::error_code& ec_out) noexcept
{
    co_return {};
}

koios::task<> 
db_impl::
may_prepare_space(const write_batch& b)
{
    const size_t serialized_size = b.serialized_size();
    const size_t page_size = m_opt.memory_page_bytes;
    const size_t page_size_80percent = static_cast<size_t>(static_cast<double>(page_size) * 0.8);
    
    if (const auto future_sz = co_await m_stat.approx_hot_data_size_bytes() + serialized_size;
        future_sz > m_opt.memory_page_bytes)
    {
        // TODO
    }
    else if (future_sz > page_size_80percent)
    {
        // TODO
    }

    co_return;
}

} // namespace frenzykv
