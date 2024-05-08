#ifndef FRENZYKV_DB_VERSION_H
#define FRENZYKV_DB_VERSION_H

#include <memory>
#include <ranges>
#include <vector>
#include <cassert>
#include <list>

#include "toolpex/ref_count.h"
#include "toolpex/move_only.h"

#include "koios/task.h"
#include "koios/task_concepts.h"
#include "koios/coroutine_shared_mutex.h"

#include "frenzykv/types.h"
#include "frenzykv/kvdb_deps.h"
#include "frenzykv/util/file_guard.h"
#include "frenzykv/util/file_center.h"

namespace frenzykv
{

class version_delta
{
public:
    decltype(auto) add_compacted_files(::std::ranges::range auto guards)
    {
        for (auto& item : guards)
        {
            m_compacted.emplace_back(::std::move(item));
        }
        return *this;
    }

    decltype(auto) add_new_files(::std::ranges::range auto guards)
    {
        for (auto& item : guards)
        {
            m_added.emplace_back(::std::move(item));
        }
        return *this;
    }

    decltype(auto) add_new_file(file_guard guard)
    {
        m_added.emplace_back(::std::move(guard));
        return *this;
    }

    const auto& compacted_files() const noexcept { return m_compacted; }
    const auto& added_files() const noexcept { return m_added; }

    bool empty() const noexcept { return m_compacted.empty() && m_added.empty(); }

private:
    ::std::vector<file_guard> m_compacted;
    ::std::vector<file_guard> m_added;
};

class version_rep
{
public:
    version_rep(::std::string_view desc_name);

    // It should be copyable, used by creating a nwe version based on an old one.
    version_rep(const version_rep& other);

    const auto& files() const noexcept { return m_files; }
    auto ref() noexcept { return m_ref++; }
    auto deref() noexcept { return m_ref--; }
    version_rep& operator+=(const version_delta& delta);
    version_rep& apply(const version_delta& delta)
    {
        return operator+=(delta);
    }

    void set_version_desc_name(::std::string name) noexcept
    {
        m_version_desc_name = ::std::move(name);
    }
    
    ::std::string_view version_desc_name() const noexcept { assert(!m_version_desc_name.empty()); return m_version_desc_name; }
    auto approx_ref_count() const noexcept { return m_ref.load(::std::memory_order_relaxed); }

private:
    ::std::vector<file_guard> m_files;
    toolpex::ref_count m_ref;
    ::std::string m_version_desc_name;
};

class version_guard : public toolpex::move_only
{
public:
    constexpr version_guard() noexcept = default;

    version_guard(version_rep* rep) noexcept
        : m_rep{ rep }
    {
        if (m_rep) m_rep->ref();
    }

    version_guard(version_rep& rep) noexcept : version_guard(&rep) { }

    ~version_guard() noexcept { release(); }

    version_guard& operator=(version_guard&& other) noexcept
    {
        release();
        m_rep = ::std::exchange(other.m_rep, nullptr);
        return *this;
    }

    version_guard(version_guard&& other) noexcept
        : m_rep{ ::std::exchange(other.m_rep, nullptr) }
    {
    }

    version_guard(const version_guard& other) noexcept
        : m_rep{ other.m_rep }
    {
        if (m_rep) m_rep->ref();
    }

    version_guard& operator=(const version_guard& other)
    {
        release();
        m_rep = other.m_rep;
        if (m_rep) m_rep->ref();
        return *this;
    }

    bool valid() const noexcept { return !!m_rep; }

    auto& rep() noexcept { assert(m_rep); return *m_rep; }
    const auto& rep() const noexcept { assert(m_rep); return *m_rep; }
    decltype(auto) version_desc_name() const noexcept { return rep().version_desc_name(); }

    auto& operator*() noexcept { return rep(); }
    const auto& operator*() const noexcept { return rep(); }

    auto* operator ->() noexcept { return &rep(); }
    const auto* operator ->() const noexcept { return &rep(); }

    version_guard& operator+=(const version_delta& delta)
    {
        rep() += delta;
        return *this;
    }

    const auto& files() const noexcept
    {
        assert(m_rep);
        return m_rep->files();
    }

private:
    void release() noexcept
    {
        if (m_rep)
        {
            m_rep->deref();
            m_rep = nullptr;
        }
    }
    
private:
    version_rep* m_rep{};
};

class version_center
{
public:
    version_center(file_center& fc) noexcept
        : m_file_center{ &fc }
    {
    }

    version_center(version_center&& other) noexcept;
    version_center& operator=(version_center&& other) noexcept;
    koios::task<version_guard> add_new_version();
    koios::task<version_guard> current_version() const noexcept;
    koios::eager_task<> load_current_version();
    koios::task<> set_current_version(version_guard v);

    //koios::task<void> GC_with(koios::awaitable_callable_concept auto async_func_file_range)
    koios::task<> GC_with(auto async_func_file_range)
    {
        namespace rv = ::std::ranges::views;
        auto lk = co_await m_modify_lock.acquire();

        for (const version_rep& out_dated_version : m_versions 
            | rv::take(m_versions.size())
            | rv::filter(is_garbage_in_mem))
        {
            co_await async_func_file_range(out_dated_version);
        }
        in_mem_GC_impl();
    }

    koios::task<> GC()
    {
        auto lk = co_await m_modify_lock.acquire();
        in_mem_GC_impl();
    }

    koios::task<size_t> size() const
    {
        auto lk = co_await m_modify_lock.acquire_shared();
        co_return m_versions.size();
    }

private:
    static bool is_garbage_in_mem(const version_rep& vg)
    {
        return vg.approx_ref_count() == 0;
    }

    void in_mem_GC_impl();

private:
    ::std::list<version_rep> m_versions;
    version_guard m_current;
    file_center* m_file_center{};

    // If you want to add or remove a version, acquire a unique lock
    // Otherwise acquire a shared lock.
    mutable koios::shared_mutex m_modify_lock;
};

} // namespace frenzykv

#endif
