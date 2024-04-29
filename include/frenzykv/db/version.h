#ifndef FRENZYKV_DB_VERSION_H
#define FRENZYKV_DB_VERSION_H

#include <memory>
#include <ranges>
#include <vector>
#include <list>

#include "toolpex/ref_count.h"
#include "toolpex/move_only.h"

#include "koios/task.h"
#include "koios/coroutine_shared_mutex.h"

#include "frenzykv/types.h"

namespace frenzykv
{

class version_delta
{
public:
    decltype(auto) add_compacted_files(::std::ranges::range auto const& ids)
    {
        m_compacted.append_range(ids);
        return *this;
    }

    decltype(auto) add_new_files(::std::ranges::range auto const& ids)
    {
        m_added.append_range(ids);
        return *this;
    }

    decltype(auto) add_new_file(file_id_t id)
    {
        m_added.emplace_back(id);
        return *this;
    }

    const auto& comapcted_files() const noexcept { return m_compacted; }
    const auto& added_files() const noexcept { return m_added; }

private:
    ::std::vector<file_id_t> m_compacted;
    ::std::vector<file_id_t> m_added;
};

class version_rep
{
public:
    const auto& files() const noexcept { return m_files; }
    auto ref() noexcept { return m_ref++; }
    auto deref() noexcept { return m_ref--; }
    version_rep operator+(const version_delta& delta) const;
    auto approx_ref_count() const noexcept { return m_ref.load(::std::memory_order_relaxed); }

private:
    version_rep(::std::ranges::range auto const& files)
        : m_files{ begin(files), end(files) }
    {
    }

private:
    ::std::vector<file_id_t> m_files;
    toolpex::ref_count m_ref;
};

class version_guard : public toolpex::move_only
{
public:
    version_guard(version_rep* rep) noexcept
        : m_rep{ rep }
    {
        m_rep->ref();
    }

    ~version_guard() noexcept
    {
        if (rep)
        {
            rep->deref();
            rep = nullptr;
        }
    }

    version_guard& operator=(version_guard&& other) noexcept
    {
        m_rep = ::std::exchange(other.m_rep, nullptr);
        return *this;
    }

    version_guard(version_guard&& other) noexcept
        : m_rep{ ::std::exchange(other.m_rep, nullptr) }
    {
    }
    
private:
    version_rep* m_rep{};
};

class version_center
{
public:
    version_center(const kvdb_deps& deps, sequence_number_t current)
        : m_deps{ &deps }, m_current{ current };
    {
    }

    void add_new_version(version_rep v);

    auto current_version() noexcept
    {
        auto lk = co_await m_modify_lock.acquire_shared();
        return version_guard{ m_current_ptr };
    }

private:
    const kvdb_deps* m_deps;
    sequence_number_t m_current;
    ::std::list<version_rep> m_versions;
    version_rep* m_current_ptr{};

    // If you want to add or remove a version, acquire a unique lock
    // Otherwise acquire a shared lock.
    koios::shared_mutex m_modify_lock;
};

} // namespace frenzykv

#endif
