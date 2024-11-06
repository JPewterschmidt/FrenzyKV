// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <filesystem>
#include <string>
#include <ranges>

#include "frenzykv/persistent/compaction_policy_oldest.h"

#include "frenzykv/table/sstable.h"

namespace fs = ::std::filesystem;
namespace r = ::std::ranges;
namespace rv = r::views;

namespace frenzykv
{

koios::task<::std::vector<file_guard>>
compaction_policy_oldest::
compacting_files(version_guard vc, level_t from) const
{
    auto result = vc.files() 
        | rv::filter(file_guard::with_level_predicator(from)) 
        | r::to<::std::vector<file_guard>>();

    r::sort(result, [](const auto& lhs, const auto& rhs) { 
        return lhs.last_write_time() < rhs.last_write_time(); 
    });

    // Drop the last one
    result.resize(result.size() - 1);
    
    // If there is no any file to be comapcted.
    if (result.empty()) co_return {};

    auto opt = m_deps->opt();

    assert(opt->allowed_level_file_number(from) >= 2);

    if (from > 1)
    {
        auto from_l_sst = co_await sstable::make(*m_deps, m_filter, co_await result.front().open_read());

        // Find overlapped tables from next level
        auto file_vec_level_next = vc.files() 
            | rv::filter(file_guard::with_level_predicator(from + 1))
            | r::to<::std::vector<file_guard>>();
        
        r::sort(file_vec_level_next, [](const auto& lhs, const auto& rhs) { 
            return lhs.last_write_time() < rhs.last_write_time(); 
        });
        r::sort(file_vec_level_next, [](const auto& lhs, const auto& rhs) { 
            return lhs.file_size() < rhs.file_size(); 
        });
        
        for (const auto& fguard : file_vec_level_next)
        {
            auto next_l_sst = co_await sstable::make(*m_deps, m_filter, co_await fguard.open_read());

            if (from_l_sst->overlapped(*next_l_sst))
            {
                result.push_back(fguard);
            }
        }
    }
    
    co_return result;
}

} // namespace frenzykv
