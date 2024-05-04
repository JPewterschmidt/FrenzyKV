#include "frenzykv/persistent/compaction_policy.h"

#include "frenzykv/persistent/compaction_policy_oldest.h"

namespace frenzykv
{

::std::unique_ptr<compaction_policy> make_default_compaction_policy()
{
    return ::std::make_unique<compaction_policy_oldest>();
}

} // namespace frenzykv
