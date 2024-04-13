#include "frenzykv/util/entry_extent.h"

namespace frenzykv
{

bool is_entry_regular(const entry_pbrep& entry)
{
    assert(entry.IsInitialized());
    return entry.has_value();
}

bool is_entry_deleting(const entry_pbrep& entry)
{
    assert(entry.IsInitialized());
    return !entry.has_value();
}

} // namespace frenzykv
