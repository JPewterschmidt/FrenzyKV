#include "util/comp.h"
#include <cstring>

namespace frenzykv
{

::std::strong_ordering 
memcmp_comparator::
operator()(::std::span<const ::std::byte> lhs, 
           ::std::span<const ::std::byte> rhs) const noexcept 
{
    const size_t min_len = ::std::min(lhs.size(), rhs.size());
    if (int indicator = ::std::memcmp(lhs.data(), rhs.data(), min_len); 
        indicator == 0)
    {
        if (lhs.size() == rhs.size()) return ::std::strong_ordering::equal;
        return (lhs.size() > rhs.size()) 
            ? ::std::strong_ordering::greater : ::std::strong_ordering::less;
    }
    else
    {
        return (indicator > 0) 
            ? ::std::strong_ordering::greater : ::std::strong_ordering::less;
    }
}

} // namespace frenzykv
