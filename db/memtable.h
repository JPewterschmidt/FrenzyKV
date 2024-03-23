#ifndef FRENZYKV_MEMTABLE_H
#define FRENZYKV_MEMTABLE_H

#include "toolpex/skip_list.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/statistics.h"
#include "frenzykv/options.h"
#include "koios/coroutine_mutex.h"
#include "entry_pbrep.pb.h"

namespace frenzykv
{

class memtable
{
public:
    memtable(const options& opt)
        : m_opt{ &opt },
          m_list(toolpex::skip_list_suggested_max_level(opt.memtable_size_bound))
    {
    }

    memtable(size_t approx_size_bound)
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
    const options* m_opt{};
    toolpex::skip_list<::std::string, ::std::string> m_list;
    mutable koios::mutex m_list_mutex;
};

} // namespace frenzykv

#endif
