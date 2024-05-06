#include <iterator>
#include <cassert>

#include "frenzykv/db/version.h"
#include "frenzykv/db/version_descriptor.h"

namespace frenzykv
{

namespace r = ::std::ranges;
namespace rv = r::views;

version_rep::version_rep()
    : m_version_desc_name{ get_version_descriptor_name() }
{
}

version_rep::version_rep(const version_rep& other)
    : m_files{ other.m_files }, 
      m_version_desc_name{ get_version_descriptor_name() }
{
}

version_rep& version_rep::operator+=(const version_delta& delta) 
{
    ::std::vector<file_guard> new_versions_files = delta.added_files();

    const auto& old_files = files();
    const auto& compacted_files = delta.compacted_files();
    ::std::set_difference(old_files.begin(), old_files.end(), 
                          compacted_files.begin(), compacted_files.end(), 
                          ::std::back_inserter(new_versions_files));

    m_files = ::std::move(new_versions_files);
    return *this;
}

koios::task<version_guard> version_center::add_new_version()
{
    auto lk = co_await m_modify_lock.acquire();
    m_current = { m_versions.emplace_back(m_versions.back()) };
    co_return m_current;
}

koios::eager_task<> version_center::load_current_version()
{
    auto lk = co_await m_modify_lock.acquire();
    assert(m_versions.empty());
    version_delta delta = co_await get_current_version(m_file_center->deps(), m_file_center);
    m_current = (m_versions.emplace_back() += delta);
}

} // namespace frenzykv
