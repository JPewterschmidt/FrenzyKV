#include <iterator>
#include <cassert>

#include "frenzykv/db/version.h"

namespace frenzykv
{

namespace r = ::std::ranges;
namespace rv = r::views;

version_rep& version_rep::operator+=(const version_delta& delta) 
{
    ::std::vector<file_guard> new_versions_files = delta.added_files();

    const auto& old_files = files();
    const auto& compacted_files = delta.compacted_files();
    ::std::set_difference(old_files.begin(), old_files.end(), 
                          compacted_files.begin(), compacted_files.end(), 
                          ::std::back_inserter(new_versions_files));

    assert(!new_versions_files.empty());
    m_files = ::std::move(new_versions_files);
    return *this;
}

koios::task<version_guard> version_center::add_new_version()
{
    auto lk = co_await m_modify_lock.acquire();
    m_current = { m_versions.emplace_back(m_versions.back()) };
    co_return m_current;
}

} // namespace frenzykv
