// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "frenzykv/write_batch.h"
#include "toolpex/functional.h"
#include <algorithm>
#include <ranges>
#include <iterator>
#include <utility>

namespace frenzykv
{

void write_batch::write(kv_entry entry)
{
    m_entries.emplace_back(::std::move(entry));
}

void write_batch::write(const_bspan key, const_bspan value)
{
    m_entries.emplace_back(first_sequence_num() + m_entries.size(), key, value);
}

void write_batch::write(::std::string_view k, ::std::string_view v)
{
    ::std::span<const char> ks{k}, vs{v};
    write(as_bytes(ks), as_bytes(vs));
}

void write_batch::write(write_batch other)
{
    auto& other_ety = other.m_entries;
    ::std::move(other_ety.begin(), other_ety.end(), 
                ::std::back_inserter(m_entries));
}

size_t write_batch::serialized_size() const
{
    namespace rv = ::std::ranges::views;
    auto each_sz = m_entries | rv::transform([](auto&& i) noexcept { return i.serialized_bytes_size(); });
    return ::std::ranges::fold_left(each_sz, 0, ::std::plus<size_t>());
}

auto write_batch::stl_style_remove(const_bspan key)
{
    return ::std::remove_if(m_entries.begin(), m_entries.end(), [key](auto&& i) { 
        ::std::string_view iter_uk{ i.key().user_key() };
        ::std::string_view pred_uk{ as_string_view(key) };
        return iter_uk == pred_uk;
    });
}

void write_batch::remove_from_db(const_bspan key)
{
    auto iter = stl_style_remove(key);
    if (::std::ranges::distance(iter, m_entries.end()) > 0)
    {
        iter->set_tomb_stone(true);
        m_entries.erase(::std::next(iter), m_entries.end());
    }
    else 
    {
        m_entries.emplace_back(first_sequence_num() + m_entries.size(), key);
    }
}

void write_batch::remove_from_db(::std::string_view key)
{
    ::std::span<const char> ks{ key };
    remove_from_db(as_bytes(ks));
}

size_t write_batch::serialize_to(bspan buffer) const
{
    const size_t result = serialized_size();
    if (buffer.size() < result)
        return 0;
    auto writable = buffer;
    for (const auto& entry : m_entries)
    {
        if (size_t occupied = entry.serialize_to(writable);
            occupied == 0) // not serialized
        {
            return 0;
        }
        else writable = writable.subspan(occupied);
    }
    return result;
}

::std::string write_batch::to_string_debug() const 
{
    return this->to_string_log();   
}

::std::string write_batch::to_string_log() const 
{
    using namespace ::std::string_literals;

    const auto entry_str = [this]{ 
        ::std::string result;
        if (count()) 
        {
            const auto& entry = *begin();
            result = entry.to_string_debug();
        }
        return result;
    }();
    
    return toolpex::lazy_string_concater{}  
        + "write_batch: ["
        +   "count = "              + count()
        + ", serialized_size = "    + serialized_size()
        + ", first_entry = ("       + entry_str + ")"
        + " ]";
}

void write_batch::set_first_sequence_num(sequence_number_t num) 
{ 
    m_seqnumber = num; 
    if (!empty()) repropogate_sequence_num();
}

void write_batch::repropogate_sequence_num()
{
    for (uint32_t i{}; i < count(); ++i)
    {
        m_entries[i].set_sequence_number(first_sequence_num() + i);
    }
}

} // namespace frenzykv
