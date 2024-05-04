#ifndef FRENZYKV_COMPACTION_POLICY_H
#define FRENZYKV_COMPACTION_POLICY_H

#include <vector>
#include <memory>

#include "frenzykv/db/version.h"
#include "fernzykv/util/file_guard.h"

namespace frenzykv
{

class compaction_policy
{
public:
    virtual ::std::vector<file_guard> 
    compacting_files(const version_center& vc, level_t from) const = 0; 
};

::std::unique_ptr<compaction_policy> make_default_compaction_policy();

} // namespace frenzykv

#endif
