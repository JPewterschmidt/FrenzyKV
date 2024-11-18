// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_TABLE_MEMTABLE_H
#define FRENZYKV_TABLE_MEMTABLE_H

#include <system_error>
#include <optional>
#include <utility>
#include <memory_resource>

#include "toolpex/skip_list.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/kvdb_deps.h"

namespace frenzykv
{

class memtable
{
public:
    using container_type = toolpex::skip_list<
        sequenced_key, kv_user_value,
        ::std::less<sequenced_key>, 
        ::std::equal_to<sequenced_key>, 
        ::std::pmr::polymorphic_allocator<::std::pair<sequenced_key, kv_user_value>>
    >;

public:
    memtable(const kvdb_deps& deps)
        : m_deps{ &deps },
          m_bound_size_bytes{ m_deps->opt()->memory_page_bytes },
          m_mbr(m_bound_size_bytes),
          m_pa(&m_mbr),
          m_list(toolpex::skip_list_suggested_max_level(m_deps->stat()->hot_data_scale_baseline()), m_pa)
    {
        assert(m_deps);
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

    size_t m_bound_size_bytes{};
    size_t m_size_bytes{};

    ::std::pmr::monotonic_buffer_resource m_mbr;
    ::std::pmr::polymorphic_allocator<::std::pair<sequenced_key, kv_user_value>> m_pa;
    container_type m_list;
};

} // namespace frenzykv

#endif
