#ifndef FRENZYKV_PROTO_KEY_CMP_H
#define FRENZYKV_PROTO_KEY_CMP_H

#include "entry_pbrep.pb.h"
#include <functional>

namespace frenzykv
{

class seq_key_less
{
public:
    bool operator<()(const seq_key& lhs, const seq_key& rhs) const noexcept
    {
        ::std::string lhs_str, rhs_str;
        lhs.SerializeToString(&lhs_str);
        rhs.SerializeToString(&rhs_str);
        return ::std::less<::std::string>{}(lhs_str, rhs_str);
    }
};

} // namespace frenzykv

#endif
