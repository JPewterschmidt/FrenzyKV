// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <filesystem>
#include <string>
#include <ranges>

#include "frenzykv/env.h"

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
    ::std::vector<file_guard> result;

    const auto& files = vc.files();
    auto files_level_from = files | rv::filter(file_guard::with_level_predicator(from));
    ::std::vector file_vec_level_from(begin(files_level_from), end(files_level_from));

    r::sort(file_vec_level_from, [](const auto& lhs, const auto& rhs) { 
        return lhs.last_write_time() < rhs.last_write_time(); 
    });
    
    auto opt = m_deps->opt();
    auto env = m_deps->env();

    assert(opt->allowed_level_file_number(from) >= 2);

    // Suppose to be empalce_range, but GCC 13 not supports it.
    for (auto& fg : file_vec_level_from 
        | rv::take(file_vec_level_from.size() - 1))
    {
        assert(fg.valid());
        result.push_back(::std::move(fg));
    }
    file_vec_level_from = {};
    
    // If there is no any file to be comapcted.
    if (result.empty()) co_return {};

    if (from != 0)
    {
        auto from_l_sst = co_await sstable::make(*m_deps, m_filter, co_await result.front().open_read(env.get()));

        // Find overlapped tables from next level
        auto files_level_next = files | rv::filter(file_guard::with_level_predicator(from + 1));
        ::std::vector file_vec_level_next(begin(files_level_next), end(files_level_next));
        
        r::sort(file_vec_level_next, [](const auto& lhs, const auto& rhs) { 
            return lhs.last_write_time() < rhs.last_write_time(); 
        });
        r::sort(file_vec_level_next, [](const auto& lhs, const auto& rhs) { 
            return lhs.file_size() < rhs.file_size(); 
        });
        
        for (const auto& fguard : file_vec_level_next)
        {
            auto next_l_sst = co_await sstable::make(*m_deps, m_filter, co_await fguard.open_read(env.get()));

            if (from_l_sst->overlapped(*next_l_sst))
            {
                result.push_back(fguard);
            }
        }
    }
    
    co_return result;
}

} // namespace frenzykv
