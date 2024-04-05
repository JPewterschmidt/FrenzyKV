#include "util/memtable_set.h"
#include "db/memtable.h"
#include "frenzykv/statistics.h"

namespace frenzykv
{

koios::task<::std::error_code> 
memtable_set::
memtable_transform()
{
    auto lk = co_await m_transform_mutex.acquire();
    if (m_imm_mem) co_return make_frzkv_out_of_range();
    m_imm_mem = ::std::make_unique<imm_memtable>(::std::move(*m_mem));
    m_mem = ::std::make_unique<memtable>(*m_deps);
    co_return {};
}

koios::task<::std::optional<entry_pbrep>> 
memtable_set::
get(const seq_key& key)
{
    auto result = co_await m_mem->get(key);
    if (result)
        co_return result;
    result = co_await m_imm_mem->get(key);
    co_return result;
}

koios::task<bool> memtable_set::full() const
{
    auto lk = co_await m_transform_mutex.acquire_shared();
    co_return m_imm_mem && co_await m_mem->full();
}

koios::task<::std::unique_ptr<imm_memtable>>
memtable_set::
get_imm_memtable()
{
    auto lk = co_await m_transform_mutex.acquire();
    co_return ::std::move(m_imm_mem);
}

koios::task<::std::pair<size_t, size_t>> 
memtable_set::
size_bytes()
{
    auto lk = co_await m_transform_mutex.acquire_shared();
    const size_t memsz = co_await m_mem->size_bytes();
    const size_t immsz = m_imm_mem ? co_await m_imm_mem->size_bytes() : 0;
    co_return { memsz, immsz };
}

koios::task<bool> 
memtable_set::
could_fit_in_mem(const write_batch& b, [[maybe_unused]] auto& unilk)
{
    const size_t b_sz = b.serialized_size();
    const size_t mem_sz = co_await m_mem->size_bytes();
    co_return b_sz + mem_sz < co_await m_mem->bound_size_bytes();
}

koios::task<bool> 
memtable_set::
could_fit_in(const write_batch& b)
{
    auto lk = co_await m_transform_mutex.acquire_shared();
    co_return co_await could_fit_in_mem(b, lk) || m_imm_mem == nullptr;
}

} // namespace frenzykv
