#ifndef FRENZYKV_TYPES_H
#define FRENZYKV_TYPES_H

#include <cstdint>
#include <span>
#include <string_view>

namespace frenzykv
{

using sequence_number_t = uint32_t;
using bspan = ::std::span<::std::byte>;
using const_bspan = ::std::span<const ::std::byte>;
using file_id_t = uint32_t;
using level_t = size_t;

::std::string_view as_string_view(const_bspan s);

} // namespace frenzykv

#endif
