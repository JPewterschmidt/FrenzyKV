#ifndef FRENZYKV_UTIL_FILE_GUARD_H
#define FRENZYKV_UTIL_FILE_GUARD_H

#include <cassert>

#include "toolpex/ref_count.h"

#include "frenzykv/types.h"

namespace frenzykv
{

class file_rep
{
public:
    constexpr file_rep() noexcept = default;

    file_rep(level_t l, file_id_t id) noexcept
        : m_level{ l }, m_fileid{ id }
    {
    }

    auto ref() { return m_ref++; }
    auto deref() { return m_ref--; }
    auto approx_ref_count() const noexcept { return m_ref.load(::std::memory_order_relaxed); }
    file_id_t file_id() const noexcept { return m_fileid; }
    level_t level() const noexcept { return m_level; }
    
    operator file_id_t() const noexcept { return file_id(); }
    operator level_t() const noexcept { return level(); }

    bool operator==(const file_rep& other) const noexcept
    {
        if (this == &other) return true;
        assert(file_id() != other.file_id());
        return false;
    }

private:
    level_t m_level{};
    file_id_t m_fileid{};
    toolpex::ref_count m_ref;
};

class file_guard
{
public:
    constexpr file_guard() noexcept = default;

    file_guard(file_rep* rep) noexcept : m_rep{ rep } { }
    file_guard(file_rep& rep) noexcept : m_rep{ &rep } { }

    ~file_guard() noexcept { release(); }

    file_guard(file_guard&& other) noexcept
        : m_rep{ ::std::exchange(other.m_rep, nullptr) }
    {
    }

    file_guard& operator=(file_guard&& other) noexcept
    {
        release();
        m_rep = ::std::exchange(other.m_rep, nullptr);
        return *this;
    }

    file_guard(const file_guard& other) noexcept
        : m_rep{ other.m_rep }
    {
        m_rep->ref();
    }

    file_guard& operator=(const file_guard& other) noexcept
    {
        release();
        m_rep = other.m_rep;
        m_rep->ref();
        return *this;
    }

    file_id_t file_id() const noexcept 
    {
        assert(m_rep);
        return m_rep->file_id();
    }

    level_t level() const noexcept 
    {
        assert(m_rep);
        return m_rep->level();
    }

    auto& rep() noexcept { assert(m_rep); return *m_rep; }
    const auto& rep() const noexcept { assert(m_rep); return *m_rep; }

    operator file_id_t() const noexcept { return file_id(); }
    operator level_t() const noexcept { return level(); }

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
    file_rep* m_rep{};
};

} // namespace frenzykv

#endif
