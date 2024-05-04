#ifndef FRENZYKV_PERSISTENT_COMPACTION_POLICY_OLDEST_H
#define FRENZYKV_PERSISTENT_COMPACTION_POLICY_OLDEST_H

#include "frenzykv/persistent/compaction_policy.h"

namespace frenzykv
{

class compaction_policy_oldest : public compaction_policy
{
public:
    ::std::vector<file_guard> 
    compacting_files(const version_guard& vc, level_t from) const override;
};

} // namespace frenzykv

#endif
