#include "frenzykv/compaction_policy_tombstone.h"

namespace frenzykv
{

koios::task<::std::vector<file_guard>>
compaction_policy_tombstone::
compacting_files(version_guard vc, level_t from) const
{

}

} // namespace frenzykv
