#include "util/memtable_set.h"
#include "db/memtable.h"
#include "frenzykv/statistics.h"
#include <system_error>
#include <memory>

namespace frenzykv
{

koios::task<::std::error_code> 
memtable_set::
memtable_transfer()
{
    auto lk = co_await m_transfer_mutex.acquire();
    if (m_imm_mem) co_return make_frzkv_out_of_range();
    m_imm_mem = ::std::make_unique<imm_memtable>(::std::move(*m_mem));
    m_mem = ::std::make_unique<memtable>(co_await global_statistics().approx_hot_data_scale());
    co_return {};
}

koios::task<entry_pbrep> 
memtable_set::
get(const ::std::string& key)
{
    auto result = co_await m_mem->get(key);
    if (result.IsInitialized())
        co_return result;
    result = co_await m_imm_mem->get(key);
    co_return result;
}

koios::task<bool> memtable_set::full() const
{
    auto lk = co_await m_transfer_mutex.acquire_shared();
    co_return m_imm_mem && co_await m_mem->full();
}

koios::task<::std::unique_ptr<imm_memtable>>
memtable_set::
get_imm_memtable()
{
    auto lk = co_await m_transfer_mutex.acquire();
    co_return ::std::move(m_imm_mem);
}

} // namespace frenzykv
