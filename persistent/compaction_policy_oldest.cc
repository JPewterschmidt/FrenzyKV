#include <filesystem>
#include <string>

#include "frenzykv/env.h"

#include "frenzykv/persistent/compaction_policy_oldest.h"

namespace fs = ::std::filesystem;

namespace frenzykv
{

::std::vector<file_guard> 
compaction_policy_oldest::
compacting_files(const version_guard& vc, level_t from) const
{
    ::std::vector<file_guard> result;

    const auto& files = vc.files();
    file_guard guard_oldest_file;
    fs::file_time_type ftime = fs::file_time_type::max();
    for (const auto& fguard : files)
    {
        if (auto lmt = fs::last_write_time(sstables_path()/fguard.name()); 
            lmt < ftime)
        {
            ftime = lmt;
            guard_oldest_file = fguard;
        }
    }
    
    result.emplace_back(::std::move(guard_oldest_file));
    return result;
}

} // namespace frenzykv
