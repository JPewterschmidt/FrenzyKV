#include "frenzykv/db/kv_entry.h"
#include "frenzykv/util/serialize_helper.h"
#include "toolpex/exceptions.h"
#include <limits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <span>

namespace frenzykv
{

using namespace toolpex;

koios::generator<kv_entry> kv_entries_from_buffer(const_bspan buffer)
{
    const ::std::byte* sentinal = buffer.data() + buffer.size();
    for (const ::std::byte* cur = buffer.data(); 
         cur && cur < sentinal; 
         cur = next_serialized_entry_beg(cur, sentinal))
    {
        auto se_span = serialized_entry(cur);
        if (se_span.empty()) continue;
        co_yield kv_entry{ se_span };
    }
}

sequenced_key::sequenced_key(const_bspan serialized_seq_key)
{
    if (serialized_seq_key.empty()) return;

    uint16_t userkey_len = decode_big_endian_from<uint16_t>(serialized_seq_key.subspan(0, sizeof(uint16_t)));

    serialized_seq_key = serialized_seq_key.subspan(user_key_length_bytes_size);
    auto userkey = serialized_seq_key.subspan(0, userkey_len);
    auto seq_number_bytes = serialized_seq_key.subspan(userkey_len, seq_bytes_size);
    m_seq = decode_big_endian_from<sequence_number_t>(seq_number_bytes);
    m_user_key = ::std::string{ reinterpret_cast<const char*>(userkey.data()), userkey.size() };
}

bool is_partial_serialized_entry(const ::std::byte* entry_beg, 
                                 const ::std::byte* sentinel)
{
    toolpex::not_implemented();
    return {};
}
    
size_t
sequenced_key::
serialize_to(::std::span<::std::byte> buffer) const noexcept 
{
    const size_t serialized_sz = static_cast<size_t>(serialized_bytes_size());
    if (buffer.size() < serialized_sz) 
        return {};

    size_t cursor{};
    const uint16_t user_key_len = static_cast<uint16_t>(m_user_key.size());
    encode_big_endian_to(user_key_len, buffer);
    cursor += user_key_length_bytes_size;
    ::std::copy(m_user_key.begin(), m_user_key.end(), 
                reinterpret_cast<char*>(buffer.data() + cursor));
    cursor += m_user_key.size();
    encode_big_endian_to(m_seq, { buffer.data() + cursor, seq_bytes_size});

    return serialized_sz;
}

::std::string sequenced_key::to_string_debug() const
{
    return toolpex::lazy_string_concater{} 
        + "sequence number: [" + sequence_number() 
        + "user key: [" + user_key();
}

size_t kv_user_value::serialized_bytes_size() const noexcept
{
    return total_length_bytes_size + (is_tomb_stone() ? 0 : m_user_value->size());
}

size_t kv_user_value::serialize_to(bspan buffer) const noexcept
{
    const size_t result = serialized_bytes_size();
    if (buffer.size() < serialized_bytes_size())
        return 0;
    uint32_t value_len = static_cast<uint32_t>(size());
    encode_big_endian_to(value_len, {buffer.data(), user_value_length_bytes_size});
    
    if (value_len)
    {
        const auto& value_rep = *m_user_value;
        ::std::copy(value_rep.begin(), value_rep.end(), 
                    reinterpret_cast<char*>(buffer.data() + user_value_length_bytes_size));
    }
    return result;
}

size_t kv_entry::
serialize_to(::std::span<::std::byte> dst) const noexcept
{
    const uint32_t total_len = static_cast<uint32_t>(serialized_bytes_size());
    if (dst.size() < total_len) 
        return {};
    
    size_t cursor{};
    ::std::span total_len_buffer{ dst.data(), total_length_bytes_size };
    encode_big_endian_to(total_len, total_len_buffer);
    cursor += total_length_bytes_size;
    m_key.serialize_to(dst.subspan(cursor));
    cursor += m_key.serialized_bytes_size();
    
    m_value.serialize_to(dst.subspan(cursor));

    return static_cast<size_t>(total_len);
}

size_t kv_entry::serialized_bytes_size() const noexcept
{
    return total_length_bytes_size 
        + m_key.serialized_bytes_size() 
        + m_value.serialized_bytes_size();
}

size_t kv_entry::serialize_append_to_string(::std::string& str) const
{
    ::std::string appended{};
    const size_t result = serialize_to(appended);
    str.append(::std::move(appended));
    return result;
}

size_t kv_entry::serialize_to(::std::string& str) const 
{
    str.clear();
    str.resize(serialized_bytes_size());
    return serialize_to(::std::span{str});
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
    if (serialized_value.empty()) return {};
    uint32_t value_len{};
    ::std::span value_len_buf{ serialized_value.data(), user_value_length_bytes_size };
    value_len = decode_big_endian_from<uint32_t>(value_len_buf);
    if (value_len == 0) return {};
    
    return {::std::string{reinterpret_cast<const char*>(serialized_value.data() + user_value_length_bytes_size), value_len}};
}

const ::std::string& 
kv_user_value::value() const 
{ 
    if (is_tomb_stone())
        throw ::std::logic_error{"kv_user_value: There's no user value data."};

    return *m_user_value; 
}

size_t kv_user_value::serialize_to(::std::string& dst) const
{
    dst.clear();
    dst.resize(serialized_bytes_size(), 0);
    return serialize_to(::std::span{dst});
}

size_t kv_user_value::serialize_append_to_string(::std::string& dst) const
{
    ::std::string temp;
    const size_t result = serialize_to(temp);
    dst.append(::std::move(temp));
    return result;
}

bool kv_user_value::operator==(const kv_user_value& other) const noexcept
{
    if (is_tomb_stone() && other.is_tomb_stone()) return true;
    if (is_tomb_stone() || other.is_tomb_stone()) return false;
    return *m_user_value == *(other.m_user_value);
}

bool sequenced_key::operator==(const sequenced_key& other) const noexcept
{
    return sequence_number() == other.sequence_number() && user_key() == other.user_key();
}

bool sequenced_key::logic_lexicographic_simulate_less(const sequenced_key& rhs) const noexcept
{
    // See also KV entry definition
    const auto& lk = user_key();
    const auto& rk = rhs.user_key();
    const auto  ls = sequence_number();
    const auto  rs = rhs.sequence_number();

    // Simulate lexicgraphical order after serialized.
    /**/ if (lk.size() < rk.size()) return true;   
    else if (lk.size() > rk.size()) return false;
    /**/ if (lk < rk)               return true;
    else if (lk > rk)               return false;

    // Simulate lexicgraphical order involving a bytes array and a integer.
    return ls < rs;
}

bool sequenced_key::operator<(const sequenced_key& rhs) const noexcept
{
    return logic_lexicographic_simulate_less(rhs);
}

::std::string kv_user_value::serialize_as_string() const
{
    ::std::string result(serialized_bytes_size(), 0);
    serialize_to(::std::span{result});
    return result;
}

size_t sequenced_key::serialize_to(::std::string& str) const 
{
    str.clear();
    str.resize(serialized_bytes_size());
    return serialize_to(::std::span{str});
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
    return decode_big_endian_from<uint32_t>({ beg, total_length_bytes_size });
}

const_bspan serialized_sequenced_key(const ::std::byte* entry_beg)
{
    const ::std::byte* userkey_len_beg = entry_beg + total_length_bytes_size;
    uint16_t userkey_len = decode_big_endian_from<uint16_t>({userkey_len_beg, user_key_length_bytes_size});
    return { userkey_len_beg, user_key_length_bytes_size + userkey_len + seq_bytes_size };
}

const_bspan serialized_user_value_from_value_len(const ::std::byte* value_len_beg)
{
    uint32_t value_len = decode_big_endian_from<uint32_t>({ value_len_beg, user_value_length_bytes_size });

    // Including the value len buffer itself
    return { value_len_beg, user_value_length_bytes_size + value_len };
}

const_bspan serialized_user_value(const ::std::byte* entry_beg)
{
    auto seq_key = serialized_sequenced_key(entry_beg);
    const ::std::byte* value_len_beg = entry_beg + total_length_bytes_size + seq_key.size();
    return serialized_user_value_from_value_len(value_len_beg);
}

size_t append_eof_to_string(::std::string& dst)
{
    static const ::std::string extra(total_length_bytes_size, 0);
    dst.append(extra);
    return extra.size();
}

size_t write_eof_to_buffer(bspan buffer)
{
    if (buffer.size() < total_length_bytes_size) return 0;
    ::std::memset(buffer.data(), 0, total_length_bytes_size);
    return total_length_bytes_size;
}

size_t sequenced_key::serialize_sequence_number_append_to(::std::string& dst) const
{
    ::std::array<char, sizeof(sequence_number_t)> buffer{};
    ::std::span seq_buffer{ buffer.data(), sizeof(m_seq) };
    encode_big_endian_to(m_seq, seq_buffer);
    dst.append(buffer.data(), buffer.size());
    return buffer.size();
}

size_t sequenced_key::serialize_user_key_append_to(::std::string& dst) const
{
    ::std::array<char, sizeof(sequence_number_t)> ukl_buffer{};
    uint16_t ukl = static_cast<uint16_t>(m_user_key.size());
    ::std::span ukl_buffer_p{ ukl_buffer.data(), sizeof(ukl) };
    encode_big_endian_to(ukl, ukl_buffer_p);
    dst.append(ukl_buffer.data(), ukl_buffer.size());
    dst.append(m_user_key);
    return user_key_length_bytes_size + m_user_key.size();
}

::std::string sequenced_key::serialize_as_string() const
{
    ::std::string result(serialized_bytes_size(), 0);
    ::std::span b{ result };
    serialize_to(::std::as_writable_bytes(b));
    return result;
}

::std::string sequenced_key::serialize_user_key_as_string() const
{
    ::std::string result(serialized_user_key_bytes_size(), 0);
    ::std::span b{ result };
    uint16_t ukl = static_cast<uint16_t>(m_user_key.size());
    toolpex::encode_big_endian_to(ukl, b);
    b = b.subspan(sizeof(ukl));
    ::std::memcpy(b.data(), m_user_key.data(), m_user_key.size());

    return result;
}

} // namespace frenzykv
