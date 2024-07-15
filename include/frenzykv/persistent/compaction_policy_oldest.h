// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_PERSISTENT_COMPACTION_POLICY_OLDEST_H
#define FRENZYKV_PERSISTENT_COMPACTION_POLICY_OLDEST_H

#include "frenzykv/persistent/compaction_policy.h"

namespace frenzykv
{

class compaction_policy_oldest : public compaction_policy
{
public:
    compaction_policy_oldest(const kvdb_deps& deps, filter_policy* filter) noexcept
        : m_deps{ &deps }, m_filter{ filter }
    {
    }

    koios::task<::std::vector<file_guard>>
    compacting_files(version_guard vc, level_t from) const override;

private:
    const kvdb_deps* m_deps{};
    filter_policy* m_filter{};
};

} // namespace frenzykv

#endif
