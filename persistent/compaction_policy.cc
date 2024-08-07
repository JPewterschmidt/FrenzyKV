// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "frenzykv/persistent/compaction_policy.h"

#include "frenzykv/persistent/compaction_policy_oldest.h"

namespace frenzykv
{

::std::unique_ptr<compaction_policy> 
make_default_compaction_policy(const kvdb_deps& deps, filter_policy* filter)
{
    return ::std::make_unique<compaction_policy_oldest>(deps, filter);
}

} // namespace frenzykv
