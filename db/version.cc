#include <iterator>
#include <cassert>

#include "frenzykv/db/version.h"

namespace frenzykv
{

using namespace r = ::std::ranges;
using namespace rv = r::views;

version_rep operator+(const version_delta& delta) const
{
    const auto& old_files = files();
    ::std::vector<file_id_t> new_versions_files;
    r::set_difference(old_files, delta.comapcted_files(), 
                      ::std::back_inserter(new_versions_files));
    assert(!new_versions_files.empty());
    return { new_versions_files };
}

void add_new_version(version_rep v)
{
    auto lk = co_await m_modify_lock.acquire();
    m_current_ptr = &m_versions.emplace_back(::std::move(v));
}

} // namespace frenzykv
