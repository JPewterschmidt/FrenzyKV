#ifndef FRENZYKV_KEY_EQ_H
#define FRENZYKV_KEY_EQ_H

#include "google/protobuf/util/message_differencer.h"

namespace frenzykv
{

class serialized_seq_key_equal_to
{
public:
    bool operator()(const seq_key& lhs, const seq_key& rhs) const noexcept
    {
        ::std::string lhsstr, rhsstr;
        lhs.SerializeToString(&lhsstr);
        rhs.SerializeToString(&rhsstr);
        return lhsstr == rhsstr;
    }
};

class seq_key_equal_to
{
public:
    bool operator()(const seq_key& lhs, const seq_key& rhs) const noexcept
    {
        return lhs.seq_number() == rhs.seq_number()
            && lhs.user_key() == rhs.user_key();
    }
};
    
} // namespace frenzykv

#endif
