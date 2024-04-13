#ifndef FRENZYKV_ENTRY_EXTENT_H
#define FRENZYKV_ENTRY_EXTENT_H

#include "frenzykv/util/proto_parse_from_inner_buffer.h"
#include "entry_pbrep.pb.h"

namespace frenzykv
{

bool is_entry_regular(const entry_pbrep& entry);
bool is_entry_deleting(const entry_pbrep& entry);

} // namespace frenzykv

#endif
