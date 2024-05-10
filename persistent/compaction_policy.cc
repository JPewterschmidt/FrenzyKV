#include "frenzykv/persistent/compaction_policy.h"

#include "frenzykv/persistent/compaction_policy_oldest.h"
#include "frenzykv/persistent/compaction_policy_tombstone.h"

namespace frenzykv
{

::std::unique_ptr<compaction_policy> 
make_default_compaction_policy(const kvdb_deps& deps, filter_policy* filter)
{
    return ::std::make_unique<compaction_policy_oldest>(deps, filter);
}

::std::unique_ptr<compaction_policy> 
make_tombstone_compaction_policy(const kvdb_deps& deps, filter_policy* filter)
{
    return ::std::make_unique<compaction_policy_tombstone>(deps, filter);
}

} // namespace frenzykv
