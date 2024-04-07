#ifndef FRENZYKV_PROTO_KEY_CMP_H
#define FRENZYKV_PROTO_KEY_CMP_H

#include "entry_pbrep.pb.h"
#include <functional>

namespace frenzykv
{

class serialized_seq_key_less
{
public:
    // Native lexicgraphical comparation of seralized data
    // needs serialization.
    bool operator()(const seq_key& lhs, const seq_key& rhs) const noexcept
    {
        ::std::string lhs_str, rhs_str;
        lhs.SerializeToString(&lhs_str);
        rhs.SerializeToString(&rhs_str);
        return ::std::less<::std::string>{}(lhs_str, rhs_str);
    }
};

class seq_key_less
{
public:
    // This function provide the same behavior 
    // of lexicgraphical comparation of seralized data 
    // without any actual serialization
    bool operator()(const seq_key& lhs, const seq_key& rhs) const noexcept
    {
        const auto& lk = lhs.user_key();
        const auto& rk = rhs.user_key();
        const uint64_t ls = lhs.seq_number();
        const uint64_t rs = rhs.seq_number();

        // Simulate lexicgraphical order after serialized.
        const bool key_less = lk.size() < rk.size() || lk < rk;

        // Simulate lexicgraphical order involving a bytes array and a integer.
        if (key_less) return true;
        else if (lk == rk) return ls < rs;
        return false;
    }
};

} // namespace frenzykv

#endif
