#ifndef FRENZYKV_MEMTABLE_H
#define FRENZYKV_MEMTABLE_H

#include "toolpex/skip_list.h"
#include "frenzykv/write_batch.h"
#include "koios/coroutine_mutex.h"
#include "entry_pbrep.pb.h"

namespace frenzykv
{

class memtable
{
public:
    memtable(size_t approx_size_bound = 65536)
        : m_list(toolpex::skip_list_suggested_max_level(approx_size_bound))
    {
    }

    koios::task<> insert(const write_batch& b);
    koios::task<> insert(write_batch&& b);

    template<typename Entry>
    koios::task<> insert(Entry&& entry)
    {
        auto lk = co_await m_list_mutex.acquire();
        co_await insert_impl(::std::forward<Entry>(entry));
    }

    koios::task<entry_pbrep> get(const ::std::string& key) const noexcept;

private:
    koios::task<> insert_impl(entry_pbrep&& entry);
    koios::task<> insert_impl(const entry_pbrep& entry);
    
private:
    toolpex::skip_list<::std::string, ::std::string> m_list;
    mutable koios::mutex m_list_mutex;
};

} // namespace frenzykv

#endif
