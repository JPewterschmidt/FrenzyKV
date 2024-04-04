#ifndef FRENZYKV_MEMTABLE_SET_H
#define FRENZYKV_MEMTABLE_SET_H

#include <memory>
#include <utility>
#include <optional>
#include <system_error>
#include "db/memtable.h"
#include "koios/task.h"
#include "koios/coroutine_shared_mutex.h"
#include "frenzykv/error_category.h"
#include "frenzykv/options.h"

namespace frenzykv
{

class memtable_set
{
public:
    memtable_set(const options& opt) noexcept
        : m_opt{ &opt }, 
          m_mem{ ::std::make_unique<memtable>(opt.memory_page_bytes) }
    {
    }

    /*! \brief  Insert a new write_batch into the memtable
     *
     *  May cause memtable, immutable memtable transfer.
     *  If immutable memtable has already been set and not been converted to disk file,
     *  this function call would failed, and return a `FRZ_KVDB_OUT_OF_RANGE`.
     *
     *  \retval FRZ_KVDB_OK insert successfully.
     *  \retval FRZ_KVDB_OUT_OF_RANGE both immutable and mutable memtable full.
     */
    template<typename Batch>
    koios::task<::std::error_code> insert(Batch&& b)
    {
        auto lk = co_await m_transform_mutex.acquire_shared();
        if (!co_await could_fit_in_mem(b, lk))
        {
            auto ec = co_await memtable_transform();
            if (ec) co_return ec; 
        }
        lk.unlock();
        co_return co_await m_mem->insert(::std::forward<Batch>(b));
    }
    
    koios::task<::std::optional<entry_pbrep>> get(const seq_key& key);
    koios::task<bool> full() const;
    koios::task<::std::unique_ptr<imm_memtable>> get_imm_memtable();
    koios::task<::std::error_code> memtable_transform();
    koios::task<::std::pair<size_t, size_t>> size_bytes();
    koios::task<bool> could_fit_in(const write_batch& b);

private:
    koios::task<bool> could_fit_in_mem(const write_batch& b, auto& unilk);

private:
    const options* m_opt;
    ::std::unique_ptr<memtable> m_mem;
    ::std::unique_ptr<imm_memtable> m_imm_mem;
    mutable koios::shared_mutex m_transform_mutex;
};

} // namespace frenzykv

#endif
