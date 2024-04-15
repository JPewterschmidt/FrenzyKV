#include "frenzykv/db/kv_entry.h"
#include <limits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <span>

namespace frenzykv
{

sequenced_key::sequenced_key(const_bspan serialized_seq_key)
{
    // TODO: need test
    uint16_t userkey_len{};
    ::std::memcpy(&userkey_len, serialized_seq_key.data(), sizeof(userkey_len));

    serialized_seq_key = serialized_seq_key.subspan(2);
    auto userkey = serialized_seq_key.subspan(0, userkey_len);
    auto seq_number_bytes = serialized_seq_key.subspan(userkey_len, 4);
    ::std::memcpy(&m_seq, seq_number_bytes.data(), sizeof(m_seq));
    m_user_key = ::std::string{ reinterpret_cast<const char*>(userkey.data()), userkey.size() };
}
    
size_t
sequenced_key::
serialize_to(::std::span<::std::byte> buffer) const noexcept 
{
    // TODO: need test
    const size_t serialized_sz = static_cast<size_t>(serialized_bytes_size());
    if (buffer.size() < serialized_sz) 
        return {};

    size_t cursor{};
    const uint16_t user_key_len = static_cast<uint16_t>(m_user_key.size());
    ::std::memcpy(buffer.data(), &user_key_len, sizeof(user_key_len));
    cursor += sizeof(user_key_len);
    ::std::copy(m_user_key.begin(), m_user_key.end(), 
                reinterpret_cast<char*>(buffer.data() + cursor));
    cursor += m_user_key.size();
    ::std::memcpy(buffer.data() + cursor, &m_seq, sizeof(m_seq));

    return serialized_sz;
}

::std::string sequenced_key::to_string_debug() const
{
    return toolpex::lazy_string_concater{} 
        + "sequence number: [" + sequence_number() 
        + "user key: [" + user_key();
}

size_t kv_entry::
serialize_to(::std::span<::std::byte> dst) const noexcept
{
    // TODO: need test
    const uint32_t total_len = static_cast<uint32_t>(serialized_bytes_size());
    if (dst.size() < total_len) 
        return {};
    
    size_t cursor{};
    ::std::memcpy(dst.data(), &total_len, sizeof(total_len));
    cursor += sizeof(total_len);
    m_key.serialize_to(dst.subspan(cursor));
    cursor += m_key.serialized_bytes_size();
    
    const uint32_t value_len = static_cast<uint32_t>(m_value.size());
    ::std::memcpy(dst.data() + cursor, &value_len, sizeof(value_len));
    cursor += sizeof(value_len);
    
    if (value_len) 
    {
        ::std::copy(m_value.value().begin(), m_value.value().end(), 
                    reinterpret_cast<char*>(dst.data() + cursor));
    }

    return static_cast<size_t>(total_len);
}

kv_user_value::kv_user_value(const kv_user_value& other)
{
    if (other.is_tomb_stone())
        return;
    m_user_value = ::std::make_unique<::std::string>(*other.m_user_value);
}

kv_user_value& kv_user_value::operator=(const kv_user_value& other)
{
    m_user_value = nullptr;
    if (other.is_tomb_stone())
        return *this;
    m_user_value = ::std::make_unique<::std::string>(*other.m_user_value);
    return *this;
}

kv_user_value 
kv_user_value::
parse(const_bspan serialized_value) 
{ 
    uint32_t value_len{};
    ::std::memcpy(&value_len, serialized_value.data(), sizeof(value_len));
    if (value_len == 0) return {};
    
    return {::std::string{reinterpret_cast<const char*>(serialized_value.data() + 4), value_len}};
}

const ::std::string& 
kv_user_value::value() const 
{ 
    if (is_tomb_stone())
        throw ::std::logic_error{"kv_user_value: There's no user value data."};

    return *m_user_value; 
}

::std::string kv_user_value::to_string_debug() const
{
    if (is_tomb_stone())
        return "kv_user_value: [tomb stone]";
    return toolpex::lazy_string_concater{} 
        + "kv_user_value: [" + value() + "]";
}

::std::string kv_entry::to_string_debug() const
{
    return toolpex::lazy_string_concater{}
        + "sequenced_key: [" + m_key.to_string_debug() + "], "
        + "value: [" + m_value.to_string_debug() + "].";
}

size_t serialized_entry_size(const ::std::byte* beg)
{
    // TODO: need test
    uint32_t result{};
    ::std::memcpy(&result, beg, sizeof(result));
    return result;
}

const_bspan serialized_sequenced_key(const ::std::byte* entry_beg)
{
    // TODO: need test
    const ::std::byte* userkey_len_beg = entry_beg + 4;
    uint16_t userkey_len{};
    ::std::memcpy(&userkey_len, userkey_len_beg, sizeof(userkey_len));
    return { userkey_len_beg, 2u + userkey_len + 4u };
}

const_bspan serialized_user_value(const ::std::byte* entry_beg)
{
    // TODO: need test
    auto seq_key = serialized_sequenced_key(entry_beg);
    const ::std::byte* value_len_beg = entry_beg + 4 + seq_key.size();
    uint32_t value_len{};
    ::std::memcpy(&value_len, value_len_beg, sizeof(value_len));
    return { value_len_beg, 4u + value_len };
}

} // namespace frenzykv
