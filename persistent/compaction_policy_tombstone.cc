// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <ranges>

#include "toolpex/exceptions.h"

#include "frenzykv/persistent/compaction_policy_tombstone.h"

namespace r = ::std::ranges;
namespace rv = r::views;

namespace frenzykv
{

koios::task<::std::vector<file_guard>>
compaction_policy_tombstone::
compacting_files(version_guard vc, level_t from) const
{
    auto file_vec_level_from = vc.files()
        | rv::filter(file_guard::with_level_predicator(from)) 
        | r::to<::std::vector<file_guard>>();

    r::sort(file_vec_level_from, [](const auto& lhs, const auto& rhs) { 
        return lhs.last_write_time() < rhs.last_write_time(); 
    });
    
    co_return file_vec_level_from;
}

} // namespace frenzykv
