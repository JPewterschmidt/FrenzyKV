#ifndef FRENZYKV_MEMTABLE_SET_H
#define FRENZYKV_MEMTABLE_SET_H

#include <memory>
#include <utility>
#include "db/memtable.h"
#include "koios/task.h"
#include "koios/coroutine_mutex.h"
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

    // TODO doc
    template<typename Batch>
    koios::task<::std::error_code> insert(Batch&& b)
    {
        if (co_await m_mem->full())
        {
            auto ec = co_await memtable_transfer();
            if (ec) co_return ec; // TODO 
        }
        co_return co_await m_mem->insert(::std::forward<Batch>(b));
    }
    
    koios::task<entry_pbrep> get(const ::std::string& key);

private:
    koios::task<::std::error_code> memtable_transfer();

private:
    const options* m_opt;
    ::std::unique_ptr<memtable> m_mem;
    ::std::unique_ptr<imm_memtable> m_imm_mem;
    mutable koios::mutex m_transfer_mutex;
};

} // namespace frenzykv

#endif
