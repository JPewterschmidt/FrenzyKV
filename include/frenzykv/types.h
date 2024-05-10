#ifndef FRENZYKV_TYPES_H
#define FRENZYKV_TYPES_H

#include <cstdint>
#include <span>
#include <string_view>

#include "frenzykv/util/uuid.h"

namespace frenzykv
{

// The first sequence number should be 1 not 0
// 0 will be a magic number ot indicate something.
using sequence_number_t = uint32_t; 
using bspan = ::std::span<::std::byte>;
using const_bspan = ::std::span<const ::std::byte>;
using file_id_t = uuid;
using level_t = int32_t;

::std::string_view as_string_view(const_bspan s);

} // namespace frenzykv

#endif
