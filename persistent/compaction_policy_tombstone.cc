#include "toolpex/exceptions.h"

#include "frenzykv/persistent/compaction_policy_tombstone.h"

namespace frenzykv
{

koios::task<::std::vector<file_guard>>
compaction_policy_tombstone::
compacting_files(version_guard vc, level_t from) const
{
    //::std::vector<file_guard> result;

    //const auto& files = vc.files();
    //auto files_level_from = files | rv::filter(file_guard::with_level_predicator(from));
    //::std::vector file_vec_level_from(begin(files_level_from), end(files_level_from));

    //r::sort(file_vec_level_from, [](const auto& lhs, const auto& rhs) { 
    //    return lhs.last_write_time() < rhs.last_write_time(); 
    //});
    //
    //auto opt = m_deps->opt();
    //auto env = m_deps->env();

    //assert(opt->allowed_level_file_number(from) >= 2);

    toolpex::not_implemented();   
    co_return {};
}

} // namespace frenzykv
