#ifndef FRENZYKV_MEMTABLE_H
#define FRENZYKV_MEMTABLE_H

#include <system_error>
#include <optional>
#include <utility>
#include "toolpex/skip_list.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/kvdb_deps.h"
#include "koios/coroutine_shared_mutex.h"

namespace frenzykv
{

class memtable
{
public:
    using container_type = toolpex::skip_list<sequenced_key, kv_user_value>;

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

    koios::task<::std::error_code> insert(write_batch b);

    koios::task<::std::optional<kv_entry>> get(const sequenced_key& key) const noexcept;
    koios::task<size_t> count() const;
    koios::task<bool> full() const;
    koios::task<size_t> bound_size_bytes() const;
    koios::task<size_t> size_bytes() const;
    koios::task<bool> could_fit_in(const write_batch& batch) const noexcept;
    koios::task<bool> empty() const;

    koios::task<container_type> get_storage();

    const kvdb_deps& deps() const noexcept { return *m_deps; }

private:
    ::std::error_code insert_impl(kv_entry&& entry);
    ::std::error_code insert_impl(const kv_entry& entry);

    bool full_impl() const;
    size_t bound_size_bytes_impl() const;
    size_t size_bytes_impl() const;
    bool could_fit_in_impl(const write_batch& batch) const noexcept;
    bool empty_impl() const noexcept;
    
private:
    const kvdb_deps* m_deps{};

    container_type m_list;

    size_t m_bound_size_bytes{};
    size_t m_size_bytes{};
    //mutable koios::shared_mutex m_list_mutex;
};

} // namespace frenzykv

#endif
