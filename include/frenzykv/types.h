#ifndef FRENZYKV_TYPES_H
#define FRENZYKV_TYPES_H

#include <cstdint>
#include <span>

namespace frenzykv
{

using sequence_number_t = uint32_t;
using bspan = ::std::span<::std::byte>;
using const_bspan = ::std::span<const ::std::byte>;

} // namespace frenzykv

#endif
