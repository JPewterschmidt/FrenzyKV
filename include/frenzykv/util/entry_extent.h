#ifndef FRENZYKV_ENTRY_EXTENT_H
#define FRENZYKV_ENTRY_EXTENT_H

#include <span>
#include "toolpex/concepts_and_traits.h"
#include "frenzykv/util/proto_parse_from_inner_buffer.h"
#include "frenzykv/db/version.h"
#include "entry_pbrep.pb.h"

namespace frenzykv
{

bool is_entry_regular(const entry_pbrep& entry);
bool is_entry_deleting(const entry_pbrep& entry);

template<toolpex::size_as_byte Byte1, toolpex::size_as_byte Byte2>
entry_pbrep* 
construct_regular_entry(entry_pbrep* entry, 
                        ::std::span<const Byte1> key, 
                        sequence_number_t seq, 
                        ::std::span<const Byte2> value)
{
    assert(entry);

    entry->mutable_key()->set_user_key(key.data(), key.size());
    entry->mutable_key()->set_seq_number(seq);
    entry->mutable_value()->set_deleted(false);
    entry->mutable_value()->set_value(value.data(), value.size());

    return entry;
}

inline entry_pbrep* 
construct_regular_entry(entry_pbrep* entry, 
                        const seq_key& k,
                        const user_value& value)
{
    assert(entry);

    *(entry->mutable_key()) = k;
    *(entry->mutable_value()) = value;

    return entry;
}

template<toolpex::size_as_byte Byte>
entry_pbrep* 
construct_deleting_entry(entry_pbrep* entry, 
                         ::std::span<const Byte> key, 
                         sequence_number_t seq)
{
    assert(entry);

    entry->mutable_key()->set_user_key(key.data(), key.size());
    entry->mutable_key()->set_seq_number(seq);
    entry->mutable_value()->set_deleted(true);

    return entry;
}

} // namespace frenzykv

#endif
