#ifndef FRENZYKV_KEY_EQ_H
#define FRENZYKV_KEY_EQ_H

#include "google/protobuf/util/message_differencer.h"

namespace frenzykv
{

class seq_key_equal_to
{
public:
    bool operator()(const seq_key& lhs, const seq_key& rhs) const noexcept
    {
        return google::protobuf::util::MessageDifferencer
            ::Equals(entry_from_buffer, example_entry);
    }
};
    
} // namespace frenzykv

#endif
