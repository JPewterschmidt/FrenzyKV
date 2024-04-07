#ifndef FRENZYKV_MEMTABLE_H
#define FRENZYKV_MEMTABLE_H

#include <system_error>
#include <optional>
#include <utility>
#include "toolpex/skip_list.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/kvdb_deps.h"
#include "koios/coroutine_shared_mutex.h"
#include "entry_pbrep.pb.h"
#include "frenzykv/util/key_cmp.h"
#include "frenzykv/util/key_eq.h"

namespace frenzykv
{

class memtable
{
public:
    memtable(const kvdb_deps& deps)
        : m_deps{ &deps },
          m_list(toolpex::skip_list_suggested_max_level(m_deps->stat()->hot_data_scale_baseline())), 
          m_bound_size_bytes{ m_deps->opt()->memory_page_bytes }
    {
        assert(m_deps);
    }

    memtable(memtable&& other) noexcept
        : m_deps { ::std::exchange(other.m_deps, nullptr) }, 
          m_list{ ::std::move(other.m_list) }, 
          m_bound_size_bytes{ other.m_bound_size_bytes }
    {
        assert(m_deps);
    }

    memtable& operator=(memtable&& other) noexcept
    {
        m_deps = ::std::exchange(other.m_deps, nullptr);
        m_list = ::std::move(other.m_list);
        m_bound_size_bytes = ::std::exchange(other.m_bound_size_bytes, 0);

        return *this;
    }

    koios::task<::std::error_code> insert(const write_batch& b);
    koios::task<::std::error_code> insert(write_batch&& b);

    koios::task<::std::optional<entry_pbrep>> get(const seq_key& key) const noexcept;
    koios::task<size_t> count() const;
    koios::task<bool> full() const;
    koios::task<size_t> bound_size_bytes() const;
    koios::task<size_t> size_bytes() const;

private:
    koios::task<::std::error_code> insert_impl(entry_pbrep&& entry);
    koios::task<::std::error_code> insert_impl(const entry_pbrep& entry);
    
private:
    const kvdb_deps* m_deps{};
    toolpex::skip_list<seq_key, ::std::string, seq_key_less, seq_key_equal_to> m_list;
    size_t m_bound_size_bytes{};
    size_t m_size_bytes{};
    mutable koios::shared_mutex m_list_mutex;
};

class imm_memtable
{
public:
    imm_memtable(memtable&& m) noexcept
        : m_mem{ ::std::move(m) }
    {
    }

    koios::task<::std::optional<entry_pbrep>> get(const seq_key& key);
    koios::task<size_t> size_bytes() const noexcept
    {
        co_return co_await m_mem.size_bytes();
    }

private:
    memtable m_mem;
};

} // namespace frenzykv

#endif
