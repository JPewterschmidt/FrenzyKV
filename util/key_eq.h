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
        //return google::protobuf::util::MessageDifferencer::Equals(lhs, rhs);
        ::std::string lhsstr, rhsstr;
        lhs.SerializeToString(&lhsstr);
        rhs.SerializeToString(&rhsstr);
        return lhsstr == rhsstr;
    }
};
    
} // namespace frenzykv

#endif
