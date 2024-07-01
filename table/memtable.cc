#include <utility>

#include "toolpex/assert.h"

#include "frenzykv/table/memtable.h"
#include "frenzykv/error_category.h"

namespace frenzykv
{

koios::task<::std::error_code> memtable::insert(write_batch b)
{
    if (!could_fit_in_impl(b))
    {
        co_return make_frzkv_out_of_range();
    }

    ::std::error_code result{};
    for (auto& item : b)
    {
        result = insert_impl(::std::move(item));
        if (result) break;
    }
    co_return result;
}

::std::error_code memtable::
insert_impl(kv_entry&& b)
{
    m_size_bytes += b.serialized_bytes_size();
    m_list.insert(::std::move(b.mutable_key()), ::std::move(b.mutable_value()));
    return {};
}

::std::error_code memtable::
insert_impl(const kv_entry& b)
{
    m_size_bytes += b.serialized_bytes_size();
    m_list[b.key()] = b.value();
    return {};
}

static 
::std::optional<kv_entry> 
table_get(auto&& list, const sequenced_key& key) noexcept
{
    ::std::optional<kv_entry> result{};
    if (auto iter = list.find_last_less_equal(key); 
        iter != list.end() && iter->first.user_key() == key.user_key())
    {
        result.emplace(iter->first, iter->second);
    }
    return result;
}

koios::task<::std::optional<kv_entry>> memtable::
get(const sequenced_key& key) const noexcept
{
    co_return table_get(m_list, key);
}

koios::task<size_t> memtable::count() const
{
    co_return m_list.size();
}

koios::task<size_t> memtable::bound_size_bytes() const
{
    toolpex_assert(m_bound_size_bytes);
    co_return bound_size_bytes_impl();
}

koios::task<bool> memtable::full() const
{
    toolpex_assert(m_bound_size_bytes);
    co_return full_impl();
}

bool memtable::full_impl() const
{
    return m_list.size() >= m_bound_size_bytes;
}

size_t memtable::bound_size_bytes_impl() const
{
    return m_bound_size_bytes;
}

size_t memtable::size_bytes_impl() const
{
    return m_size_bytes;
}

koios::task<size_t> memtable::size_bytes() const
{
    co_return size_bytes_impl();
}

bool memtable::could_fit_in_impl(const write_batch& batch) const noexcept
{
    const size_t batch_sz = batch.serialized_size();
    return batch_sz + size_bytes_impl() <= bound_size_bytes_impl();
}

bool memtable::empty_impl() const noexcept
{
    return m_list.empty();
}

koios::task<bool> memtable::could_fit_in(const write_batch& batch) const noexcept
{
    co_return could_fit_in_impl(batch);
}

koios::task<bool> memtable::empty() const
{
    co_return empty_impl();
}

koios::task<typename memtable::container_type> memtable::get_storage() 
{ 
    co_return ::std::move(m_list); 
}

} // namespace frenzykv
