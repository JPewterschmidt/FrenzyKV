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

static seq_key make_seq_key(const_bspan user_key, sequence_number_t seq)
{
    seq_key result{};
    result.set_user_key(/*static_cast<const char*>*/(user_key.data()), user_key.size());
    result.set_seq_number(seq);
    return result;
}

db_impl::db_impl(::std::string dbname, const options& opt)
    : m_dbname{ ::std::move(dbname) }, 
      m_deps{ opt },
      m_version{ m_deps },
      m_log{ m_deps, "0001-test.frzkvlog" }, 
      m_memset{ m_deps }
{
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
    sequence_number_t seq = co_await m_version.last_sequence_number();
    const seq_key skey = make_seq_key(key, seq);
    auto result_opt = co_await m_memset.get(skey);
    if (result_opt) co_return result_opt;

    co_return {};
}

koios::task<> 
db_impl::
may_prepare_space(const write_batch& b)
{
    const size_t serialized_size = b.serialized_size();
    const size_t page_size = m_deps.opt()->memory_page_bytes;
    const size_t page_size_80percent = static_cast<size_t>(static_cast<double>(page_size) * 0.8);
    
    if (const auto future_sz = co_await m_stat.approx_hot_data_size_bytes() + serialized_size;
        future_sz > m_deps.opt()->memory_page_bytes)
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
