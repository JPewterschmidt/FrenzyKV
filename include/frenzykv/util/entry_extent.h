#ifndef FRENZYKV_ENTRY_EXTENT_H
#define FRENZYKV_ENTRY_EXTENT_H

#include "frenzykv/util/proto_parse_from_inner_buffer.h"
#include "frenzykv/db/version.h"
#include "entry_pbrep.pb.h"

namespace frenzykv
{

bool is_entry_regular(const entry_pbrep& entry);
bool is_entry_deleting(const entry_pbrep& entry);
entry_pbrep* construct_regular_entry(entry_pbrep* entry, const_bspan key, sequence_number_t seq, const_bspan value);
entry_pbrep* construct_deleting_entry(entry_pbrep* entry, const_bspan key, sequence_number_t seq);

} // namespace frenzykv

#endif
