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
        : m_opt{ &opt }
    {
    }

    template<typename Batch>
    koios::task<::std::error_code> insert(Batch&& b)
    {
        auto ec = co_await m_mem->insert(::std::forward<Batch>(b));
        if (ec == make_frzkv_out_of_range())
        {
            co_await memtable_transfer();
        }
        co_return ec;
    }
    
    koios::task<entry_pbrep> get(const ::std::string& key);

private:
    koios::task<> memtable_transfer();

private:
    ::std::unique_ptr<memtable> m_mem;
    ::std::unique_ptr<imm_memtable> m_imm_mem;
    mutable koios::mutex m_transfer_mutex;
    const options* m_opt;
};

} // namespace frenzykv

#endif
