#ifndef FRENZYKV_KV_ENTRY_H
#define FRENZYKV_KV_ENTRY_H

#include <string>
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

class sequenced_key
{
public:   
    sequenced_key(sequence_number_t seq, ::std::string key) noexcept
        : m_seq{ seq }, m_user_key{ ::std::move(key) }
    {
    }

    sequenced_key(sequence_number_t seq, const_bspan key) noexcept
        : m_seq{ seq }, 
          m_user_key{ toolpex::lazy_string_concater{} + ::std::string_view{key} }
    {
    }

    sequenced_key(const_bspan serialized_seq_key);
    
    auto sequence_number() const noexcept { return m_seq; }
    void set_sequence_number(sequence_number_t num) noexcept { m_seq = num; }
    void set_user_key(::std::string v) noexcept { m_user_key = ::std::move(v); }
    const auto& user_key() const noexcept { return m_user_key; }
    bool serialize_to(::std::span<::std::byte> buffer) const noexcept;
    size_t serialized_bytes_size() const noexcept
    {
        return sizeof(m_seq) + 2 + m_user_key.size();
    }

private:
    sequence_number_t m_seq{};
    ::std::string m_user_key{};
};

class kv_entry
{
public:
    kv_entry(sequence_number_t seq, auto key, ::std::string value) noexcept
        : m_key(seq, ::std::move(key)), m_value{ ::std::move(value) }
    {
    }

    ::std::string& mutable_value() noexcept { return m_value; }
    const ::std::string& value() const noexcept { return m_value; }
    const sequenced_key& key() const noexcept { return m_key; }
    bool serialize_to(::std::span<::std::byte> dst) const noexcept;
    size_t serialized_bytes_size() const noexcept
    {
        return sizeof(m_key) 
            + m_key.serialized_bytes_size()  // sequenced_key
            + 4 + m_value.size();            // user value length + user value
    }
    
private:
    sequenced_key m_key{};
    ::std::string m_value{};
};

class serialized_sequenced_key_less
{
public:
    bool operator()(const_bspan lhs, const_bspan rhs) const noexcept
    {
        // See also KV entry definition
        return ::std::string_view{ lhs } < ::std::string_view{ rhs };
    }
};

size_t serialized_entry_size(const ::std::byte* entry_beg);
const_bspan serialized_sequenced_key(const ::std::byte* entry_beg);
const_bspan serialized_user_value(const ::std::byte* entry_beg);

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
        const sequence_number_t ls = lhs.sequence_number();
        const sequence_number_t rs = rhs.sequence_number();

        // Simulate lexicgraphical order after serialized.
        const bool key_less = lk.size() < rk.size() || lk < rk;

        // Simulate lexicgraphical order involving a bytes array and a integer.
        if (key_less) return true;
        else if (lk == rk) return ls < rs;
        return false;
    }
};

#endif
