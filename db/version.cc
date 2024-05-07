#include <iterator>
#include <cassert>

#include "frenzykv/db/version.h"
#include "frenzykv/db/version_descriptor.h"

namespace frenzykv
{

namespace r = ::std::ranges;
namespace rv = r::views;
namespace fs = ::std::filesystem;

version_rep::version_rep(::std::string_view desc_name)
    : m_version_desc_name{ desc_name }
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

    auto old_files = files();
    auto compacted_files = delta.compacted_files();
    r::sort(old_files);
    r::sort(compacted_files);

    ::std::set_difference(old_files.begin(), old_files.end(), 
                          compacted_files.begin(), compacted_files.end(), 
                          ::std::back_inserter(new_versions_files));

    m_files = ::std::move(new_versions_files);
    return *this;
}

koios::task<version_guard> version_center::add_new_version()
{
    auto lk = co_await m_modify_lock.acquire();
    auto& new_ver = m_versions.emplace_back(m_versions.back());
    new_ver.set_version_desc_name(get_version_descriptor_name());
    m_current = { new_ver };
    co_return m_current;
}

koios::eager_task<> version_center::load_current_version()
{
    auto lk = co_await m_modify_lock.acquire();
    assert(m_versions.empty());

    const auto& deps = m_file_center->deps();

    // Load current version
    version_delta delta = co_await get_current_version(deps, m_file_center);
    m_current = (m_versions.emplace_back(*co_await current_descriptor_name(deps)) += delta);
    const ::std::string_view cvd_name = m_current.version_desc_name();
    
    // Load other versions for GC
    for (const auto& dir_entry : fs::directory_iterator(version_path()))
    {
        if (const auto name = dir_entry.path().filename().string(); 
            name != cvd_name && is_version_descriptor_name(name))
        {
            m_versions.emplace_front(name) += co_await get_version(deps, name, m_file_center);
        }
    }
}

} // namespace frenzykv
