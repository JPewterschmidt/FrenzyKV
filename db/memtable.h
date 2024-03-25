#ifndef FRENZYKV_MEMTABLE_H
#define FRENZYKV_MEMTABLE_H

#include <system_error>
#include "toolpex/skip_list.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/statistics.h"
#include "frenzykv/options.h"
#include "koios/coroutine_shared_mutex.h"
#include "entry_pbrep.pb.h"
#include "util/key_cmp.h"
#include "util/key_eq.h"

namespace frenzykv
{

class memtable
{
public:
    memtable(const options& opt)
        : m_opt{ &opt },
          m_list(toolpex::skip_list_suggested_max_level(opt.memtable_size_bound)), 
          m_bound{ opt.memtable_size_bound }
    {
    }

    memtable(memtable&& other) noexcept
        : m_opt{ other.m_opt }, 
          m_list{ ::std::move(other.m_list) }, 
          m_bound{ other.m_bound }
    {
    }

    memtable& operator=(memtable&& other) noexcept
    {
        m_opt = other.m_opt;
        m_list = ::std::move(other.m_list);
        m_bound = ::std::exchange(other.m_bound, 0);

        return *this;
    }

    memtable(size_t approx_size_bound)
        : m_list(toolpex::skip_list_suggested_max_level(approx_size_bound)), 
          m_bound{ approx_size_bound }
    {
    }

    koios::task<::std::error_code> insert(const write_batch& b);
    koios::task<::std::error_code> insert(write_batch&& b);

    template<typename Entry>
    koios::task<::std::error_code> insert(Entry&& entry)
    {
        auto lk = co_await m_list_mutex.acquire();
        co_await insert_impl(::std::forward<Entry>(entry));
    }

    koios::task<entry_pbrep> get(const seq_key& key) const noexcept;
    koios::task<size_t> count() const;
    koios::task<bool> full() const;
    koios::task<size_t> bound() const;

private:
    koios::task<::std::error_code> insert_impl(entry_pbrep&& entry);
    koios::task<::std::error_code> insert_impl(const entry_pbrep& entry);
    
private:
    const options* m_opt{};
    toolpex::skip_list<seq_key, ::std::string, seq_key_less, seq_key_equal_to> m_list;
    size_t m_bound{};
    mutable koios::shared_mutex m_list_mutex;
};

class imm_memtable
{
public:
    imm_memtable(memtable&& m) noexcept
        : m_mem{ ::std::move(m) }
    {
    }

    koios::task<entry_pbrep> get(const seq_key& key);

private:
    memtable m_mem;
};

} // namespace frenzykv

#endif
