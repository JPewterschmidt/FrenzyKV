#include "frenzykv/persistent/compaction_policy.h"

namespace frenzykv
{

::std::unique_ptr<compaction_policy> make_default_compaction_policy()
{
    return {};
}

} // namespace frenzykv
