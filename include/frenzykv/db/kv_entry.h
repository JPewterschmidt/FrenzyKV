#ifndef FRENZYKV_KV_ENTRY_H
#define FRENZYKV_KV_ENTRY_H

#include <string>
#include <memory>
#include <string_view>
#include <functional>
#include "frenzykv/types.h"
#include "toolpex/functional.h"

/*      ----------------------------------------------------------------------------|--------------|
 *      |4B           |2B              |var length  |4B  |4B           |var length  |              |
 *      ----------------------------------------------------------------------------|  Next entry  |
 *      |total length |user key length |user key    |Seq |value length |value       |              |
 *      ----------------------------------------------------------------------------|~~~~~~~~~~~~~~|
 *                    | serialized sequenced key         | serialized value         |                      
 *                    ---------------------------------------------------------------
 *      ^                                                                            ^
 *      |                                                                            |
 *      The begging position    +    the value of total length     =     The begging position of the next entry
 *                                                   
 *
 * After serialization, the content should be follow little endian.
 *  
 * total length
 *      The *total length* means the sum of all fileds' length in a single entry,
 *      equals to 
 *      4 + 2 + the value of key length + 4 + 4 + the value of value length.
 *            
 *      This filed could make file seeking eaiser.
 *
 * serialized sequenced key
 *      user key length
 *          the length of the user key
 *      user key
 *      Seq
 *          the sequence number, Little endian will hold the compare order before and after serialization.
 */

namespace frenzykv
{

// Functions below are usually used when dealing with file IO and deserialization.

/*! \brief  Get the total length of a entry in a serialized memory.
 *
 *  This function will noly read the first 4 bytes pointed by the `entry_beg` parameter.
 *  So make sure there's at least 4 bytes readable start at `entry_beg`.
 */
size_t serialized_entry_size(const ::std::byte* entry_beg);
const_bspan serialized_sequenced_key(const ::std::byte* entry_beg);
const_bspan serialized_user_value(const ::std::byte* entry_beg);

/*! \brief  Get the pointer point to the first byte of the next entry in serialized bytes.
 *
 *  We assume that there are no any gap between any two of entries.
 *  And after the last entry, there's still more than 4 bytes of memory that filled with 0 indicates the end of file.
 *  4 continuous zero-filled memory could be deserialized by `serialized_entry_size()` means 0 length entry.
 *
 *  \retval pointer_not_equals_to_nullptr the pointer point to the first byte of the next entry in serialized bytes.
 *  \retval nullptr there is no entry could be consumed.
 *  \param end the sentinal pointer, if the result-ready value are exceeds than `end`, nullptr will be returned.
 */
inline const ::std::byte* next_serialized_entry(const ::std::byte* entry_beg, const ::std::byte* end = nullptr)
{
    const size_t sz = serialized_entry_size(entry_beg);
    if (sz == 0 || entry_beg + sz >= end) return nullptr;
    return entry_beg + sz;
}

class sequenced_key
{
public:   
    constexpr sequenced_key() noexcept = default;

    sequenced_key(sequence_number_t seq, ::std::string key) noexcept
        : m_seq{ seq }, m_user_key{ ::std::move(key) }
    {
    }

    sequenced_key(sequence_number_t seq, const_bspan key) noexcept
        : m_seq{ seq }, 
          m_user_key{ toolpex::lazy_string_concater{} + as_string_view(key) }
    {
    }

    sequenced_key(const_bspan serialized_seq_key);
    sequenced_key(sequenced_key&& other) noexcept = default;
    sequenced_key& operator=(sequenced_key&& other) noexcept = default;
    sequenced_key(const sequenced_key&) = default;
    sequenced_key& operator=(const sequenced_key&) = default;
    
    auto sequence_number() const noexcept { return m_seq; }
    void set_sequence_number(sequence_number_t num) noexcept { m_seq = num; }
    void set_user_key(::std::string v) noexcept { m_user_key = ::std::move(v); }
    const auto& user_key() const noexcept { return m_user_key; }
    size_t serialize_to(::std::span<::std::byte> buffer) const noexcept;
    size_t serialize_to(::std::span<char> buffer) const noexcept
    {
        return serialize_to(::std::as_writable_bytes(buffer));
    }

    size_t serialize_to(::std::string& str) const noexcept
    {
        str.resize(serialized_bytes_size());
        return serialize_to(::std::span{str});
    }
    
    size_t serialized_bytes_size() const noexcept
    {
        return sizeof(m_seq) + 2 + m_user_key.size();
    }

    ::std::string to_string_debug() const;

    bool operator==(const sequenced_key& other) const noexcept
    {
        return sequence_number() == other.sequence_number() && user_key() == other.user_key();
    }

private:
    sequence_number_t m_seq{};
    ::std::string m_user_key{};
};

class kv_user_value
{
public:
    constexpr kv_user_value() noexcept = default;

    kv_user_value(::std::string val)
        : m_user_value{ ::std::make_unique<::std::string>(::std::move(val)) }
    {
    }

    kv_user_value(kv_user_value&&) noexcept = default;
    kv_user_value& operator=(kv_user_value&&) noexcept = default;
    kv_user_value(const kv_user_value&);
    kv_user_value& operator=(const kv_user_value&);

    void set_tomb_stone(bool v = true) noexcept { m_user_value = nullptr; }
    bool is_tomb_stone() const noexcept { return !m_user_value; }
    void set(::std::string v) { m_user_value = ::std::make_unique<::std::string>(::std::move(v)); }
    const ::std::string& value() const;
    ::std::string to_string_debug() const;
    size_t size() const noexcept { return m_user_value ? m_user_value->size() : 0; }

    bool operator==(const kv_user_value& other) const noexcept
    {
        if (is_tomb_stone() && other.is_tomb_stone()) return true;
        if (is_tomb_stone() || other.is_tomb_stone()) return false;
        return *m_user_value == *(other.m_user_value);
    }

    static kv_user_value parse(const_bspan serialized_value);

private:
    ::std::unique_ptr<::std::string> m_user_value{};
};

class kv_entry
{
public:
    constexpr kv_entry() noexcept = default;
    kv_entry(kv_entry&&) noexcept = default;
    kv_entry& operator=(kv_entry&&) noexcept = default;
    kv_entry(const kv_entry&) = default;
    kv_entry& operator=(const kv_entry&) = default;

    kv_entry(sequence_number_t seq, auto key) 
        : m_key(seq, ::std::move(key))
    {
    }

    kv_entry(sequence_number_t seq, auto key, ::std::string value) 
        : m_key(seq, ::std::move(key)), m_value{ ::std::move(value) }
    {
    }

    kv_entry(sequence_number_t seq, auto key, const_bspan value) 
        : kv_entry(seq, ::std::move(key), toolpex::lazy_string_concater{} + as_string_view(value))
    {
    }

    kv_entry(sequenced_key seqk, kv_user_value val)
        : m_key{ ::std::move(seqk) }, m_value{ ::std::move(val) }
    {
    }

    kv_entry(const_bspan serialized_entry)
        : m_key{ serialized_sequenced_key(serialized_entry.data()) }, 
          m_value{ kv_user_value::parse(serialized_user_value(serialized_entry.data())) }
    {
    }

    auto& mutable_value()               noexcept { return m_value; }
    auto& mutable_key()                 noexcept { return m_key; }
    void set_value(::std::string value) noexcept { m_value.set(::std::move(value)); }
    void set_tomb_stone(bool v = true)  noexcept { m_value.set_tomb_stone(v); }
    bool is_tomb_stone() const          noexcept { return m_value.is_tomb_stone(); }
    
    const auto& value() const noexcept { return m_value; }
    const auto& key()   const noexcept { return m_key; }

    void   set_sequence_number(sequence_number_t seq) noexcept { m_key.set_sequence_number(seq); }
    size_t serialize_to(::std::span<::std::byte> dst) const noexcept;
    size_t serialize_to(::std::span<char> dst) const noexcept
    {
        return serialize_to(::std::as_writable_bytes(dst));
    }

    size_t serialize_to(::std::string& str) const 
    {
        str.resize(serialized_bytes_size());
        return serialize_to(::std::span{str});
    }

    size_t serialized_bytes_size() const noexcept
    {
        return sizeof(m_key) 
            + m_key.serialized_bytes_size()  // sequenced_key
            + 4 + m_value.size();            // user value length + user value
    }

    ::std::string to_string_debug() const;

    bool operator==(const kv_entry& other) const noexcept
    {
        return key() == other.key() && value() == other.value();
    }
    
private:
    sequenced_key m_key;
    kv_user_value m_value;
};

class serialized_sequenced_key_less
{
public:
    bool operator()(const_bspan lhs, const_bspan rhs) const noexcept
    {
        // See also KV entry definition
        return (*this)(as_string_view(lhs), as_string_view(rhs));
    }

    bool operator()(::std::string_view lhs, ::std::string_view rhs) const noexcept
    {
        return lhs < rhs;
    }
};

} // namespace frenzykv

template<>
class std::less<frenzykv::sequenced_key>
{
public:
    bool operator()(const frenzykv::sequenced_key& lhs, 
                    const frenzykv::sequenced_key& rhs) const noexcept
    {
        // See also KV entry definition
        const auto& lk = lhs.user_key();
        const auto& rk = rhs.user_key();
        const auto  ls = lhs.sequence_number();
        const auto  rs = rhs.sequence_number();

        // Simulate lexicgraphical order after serialized.
        const bool key_less = lk.size() < rk.size() || lk < rk;

        // Simulate lexicgraphical order involving a bytes array and a integer.
        if (key_less) return true;
        else if (lk == rk) return ls < rs;
        return false;
    }
};

#endif
