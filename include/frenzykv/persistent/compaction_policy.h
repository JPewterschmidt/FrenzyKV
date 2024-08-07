// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_COMPACTION_POLICY_H
#define FRENZYKV_COMPACTION_POLICY_H

#include <vector>
#include <memory>

#include "koios/task.h"

#include "frenzykv/kvdb_deps.h"

#include "frenzykv/db/filter.h"
#include "frenzykv/db/version.h"
#include "frenzykv/util/file_guard.h"

namespace frenzykv
{

class compaction_policy
{
public:
    virtual ~compaction_policy() noexcept {}

    virtual koios::task<::std::vector<file_guard>>
    compacting_files(version_guard vc, level_t from) const = 0; 
};

::std::unique_ptr<compaction_policy> 
make_default_compaction_policy(const kvdb_deps& deps, filter_policy* filter);

} // namespace frenzykv

#endif
