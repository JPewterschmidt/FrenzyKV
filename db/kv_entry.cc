#include "frenzykv/db/kv_entry.h"
#include <limits>
#include <cstdint>

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
    ::std::memcpy(&m_sqe, seq_number_bytes.data(), sizeof(m_seq));
    m_user_key = ::std::string{ reinterpret_cast<const char*>(userkey.data()), userkey.size() };
}
    
bool 
sequenced_key::
serialize_to(::std::span<::std::byte> buffer) const noexcept 
{
    // TODO: need test
    if (buffer.size() < serialized_bytes_size()) 
        return false;

    size_t cursor{};
    const uint16_t user_key_len = static_cast<uint16_t>(m_user_key.size());
    ::std::memcpy(buffer.data(), &user_key_len, sizeof(user_key_len));
    cursor += sizeof(user_key_len);
    ::std::copy(m_user_key.begin(), m_user_key.end(), 
                buffer.data() + cursor);
    cursor += m_user_key.size();
    ::std::memcpy(buffer.data() + cursor, &m_seq, sizeof(m_seq));

    return true;
}

bool kv_entry::
serialize_to(::std::span<::std::byte> dst) const noexcept
{
    // TODO: need test
    const uint32_t total_len = serialized_bytes_size();
    if (dst.size() < total_len) 
        return false;
    
    size_t cursor{};
    ::std::memcpy(dst.data(), &total_len, sizeof(total_len));
    cursor += sizeof(total_len);
    m_key.serialize_to(dst.subspan(cursor));
    cursor += m_key.serialized_bytes_size();
    
    const uint32_t value_len = static_cast<uint32_t>(m_value.size());
    ::std::memcpy(dst.data() + cursor, &value_len, sizeof(value_len));
    cursor += sizeof(value_len);
    
    ::std::copy(m_value.begin(), m_value.end(), 
                dst.data() + cursor);

    return true;
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
    const ::std::byte* userkey_beg = userkey_len_beg + userkey_len;
    return { userkey_len_beg, 2 + userkey_len + 4 };
}

const_bspan serialized_user_value(const ::std::byte* entry_beg)
{
    // TODO: need test
    auto seq_key = serialized_seq_key(entry_beg);
    const ::std::byte* value_len_beg = 4 + seq_key.size();
    uint32_t value_len{};
    ::std::memcpy(&value_len, value_len_beg, sizeof(value_len));
    return { value_len_beg + 4, value_len };
}

} // namespace frenzykv
