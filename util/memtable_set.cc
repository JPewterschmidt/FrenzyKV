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

} // namespace frenzykv
