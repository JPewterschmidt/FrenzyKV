#include <filesystem>
#include <string>

#include "frenzykv/env.h"

#include "frenzykv/persistent/compaction_policy_oldest.h"
#include "frenzykv/persistent/sstable.h"

namespace fs = ::std::filesystem;

namespace frenzykv
{

koios::task<::std::vector<file_guard>>
compaction_policy_oldest::
compacting_files(const version_guard& vc, level_t from) const
{
    ::std::vector<file_guard> result;

    const auto& files = vc.files();
    file_guard guard_oldest_file;
    fs::file_time_type ftime = fs::file_time_type::max();

    // Find oldest file from level `from`
    for (const auto& fguard : files)
    {
        if (fguard.level() != from)
            continue;
        
        if (auto lmt = fs::last_write_time(sstables_path()/fguard.name()); 
            lmt < ftime)
        {
            ftime = lmt;
            guard_oldest_file = fguard;
        }
    }
    auto fp = result.emplace_back(::std::move(guard_oldest_file)).open_read(m_deps->env().get());
    sstable from_l_sst(*m_deps, m_filter, fp.get());
    [[maybe_unused]] bool pr = co_await from_l_sst.parse_meta_data(); assert(pr);

    // Find overlapped tables from next level
    for (const auto& fguard : files)
    {
        if (fguard.level() != from + 1)
            continue;
        
        ::std::unique_ptr<random_readable> fp = fguard.open_read(m_deps->env().get());
        sstable next_l_sst(*m_deps, m_filter, fp.get());
        co_await next_l_sst.parse_meta_data();

        if (from_l_sst.overlapped(next_l_sst))
        {
            result.push_back(fguard);
        }
    }
    
    co_return result;
}

} // namespace frenzykv
