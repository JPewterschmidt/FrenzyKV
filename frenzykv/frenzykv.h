#ifndef FRENZYKV_FRENZYKV_H
#define FRENZYKV_FRENZYKV_H

#include <span>
#include <cstddef>

namespace frenzykv
{
    using bspan = ::std::span<::std::byte>;
    using const_bspan = ::std::span<const ::std::byte>;
}

#endif
