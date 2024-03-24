#ifndef FRENZYKV_MEMTABLE_SET_H
#define FRENZYKV_MEMTABLE_SET_H

#include <memory>
#include <utility>
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
          m_mem{ ::std::make_unique<memtable>(opt.memtable_size_bound) }
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
        if (co_await m_mem->full())
        {
            auto ec = co_await memtable_transfer();
            if (ec) co_return ec; 
        }
        co_return co_await m_mem->insert(::std::forward<Batch>(b));
    }
    
    koios::task<entry_pbrep> get(const ::std::string& key);
    koios::task<bool> full() const;
    koios::task<::std::unique_ptr<imm_memtable>> get_imm_memtable();

private:
    koios::task<::std::error_code> memtable_transfer();

private:
    const options* m_opt;
    ::std::unique_ptr<memtable> m_mem;
    ::std::unique_ptr<imm_memtable> m_imm_mem;
    mutable koios::shared_mutex m_transfer_mutex;
};

} // namespace frenzykv

#endif
