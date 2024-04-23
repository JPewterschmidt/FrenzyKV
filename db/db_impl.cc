#include "frenzykv/db/db_impl.h"
#include "frenzykv/util/multi_dest_record_writer.h"
#include "frenzykv/util/stdout_debug_record_writer.h"
#include "frenzykv/log/logger.h"
#include "frenzykv/statistics.h"
#include "frenzykv/error_category.h"
#include "frenzykv/options.h"
#include <cassert>

namespace frenzykv
{

db_impl::db_impl(::std::string dbname, const options& opt)
    : m_dbname{ ::std::move(dbname) }, 
      m_deps{ opt },
      m_log{ m_deps, m_deps.env()->get_seq_writable(prewrite_log_dir().path()/"0001-test.frzkvlog") }, 
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

    co_await m_log.write(batch);
    co_await m_memset.insert(::std::move(batch));
    auto [total_count, _] = co_await m_deps.stat()->increase_hot_data_scale(batch.count(), serialized_size);
    (void)_;

    co_return total_count;
}

koios::task<::std::optional<kv_entry>> 
db_impl::get(const_bspan key, ::std::error_code& ec_out) noexcept
{
    const sequenced_key skey = co_await this->make_query_key(key);
    auto result_opt = co_await m_memset.get(skey);
    if (result_opt) co_return result_opt;

    co_return {};
}

koios::task<sequenced_key> db_impl::make_query_key(const_bspan userkey)
{
    toolpex::not_implemented();
    co_return {};
}

} // namespace frenzykv
