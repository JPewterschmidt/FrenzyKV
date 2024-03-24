#include "frenzykv/write_batch.h"
#include "util/proto_parse_from_inner_buffer.h"
#include "toolpex/functional.h"
#include <algorithm>
#include <ranges>
#include <iterator>
#include <utility>

namespace frenzykv
{

void write_batch::write(const_bspan key, const_bspan value)
{
    entry_pbrep entry;
    entry.mutable_key()->set_user_key(key.data(), key.size());
    entry.mutable_key()->set_seq_number(0);
    entry.set_value(value.data(), value.size());
    m_entries.push_back(::std::move(entry));
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
    auto each_sz = m_entries | rv::transform([](auto&& i) noexcept { return i.ByteSizeLong(); });
    return ::std::ranges::fold_left(each_sz, 0, ::std::plus<size_t>());
}

auto write_batch::stl_style_remove(const_bspan key)
{
    return ::std::remove_if(m_entries.begin(), m_entries.end(), [key](auto&& i) { 
        const auto* predk = key.data();
        const auto& seqk = i.key();
        const auto* iterk = seqk.user_key().c_str();
        const size_t sz = ::std::min(seqk.user_key().size(), key.size());
        return ::std::memcmp(predk, iterk, sz) == 0;
    });
}

void write_batch::remove_from_db(const_bspan key)
{
    auto iter = stl_style_remove(key);
    if (::std::ranges::distance(iter, m_entries.end()) > 0)
    {
        iter->clear_value();
        m_entries.erase(::std::next(iter), m_entries.end());
    }
    else 
    {
        entry_pbrep entry;
        entry.mutable_key()->set_user_key(key.data(), key.size());
        entry.mutable_key()->set_seq_number(0);
        m_entries.push_back(::std::move(entry));
    }
}

void write_batch::remove_from_db(::std::string_view key)
{
    ::std::span<const char> ks{ key };
    remove_from_db(as_bytes(ks));
}

size_t write_batch::serialize_to_array(bspan buffer) const
{
    const size_t result = serialized_size();
    if (buffer.size() < result)
        return 0;
    auto writable = buffer;
    for (const auto& entry : m_entries)
    {
        if (!entry.SerializeToArray(writable.data(), (int)writable.size()))
            return 0;
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
            result = entry.DebugString();
            ::std::ranges::replace(result, '\n', ',');
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

} // namespace frenzykv
