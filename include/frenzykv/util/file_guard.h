#ifndef FRENZYKV_UTIL_FILE_GUARD_H
#define FRENZYKV_UTIL_FILE_GUARD_H

#include "toolpex/ref_count.h"

#include "frenzykv/types.h"

namespace frenzykv
{

class file_rep
{
public:
    constexpr file_rep() noexcept = default;
    auto ref() { return m_ref++; }
    auto deref() { return m_ref--; }
    auto approx_ref_count() const noexcept { return m_ref.load(::std::memory_order_relaxed); }
    file_id_t file_id() const noexcept { return m_fileid; }
    level_t level() const noexcept { return m_level; }

private:
    file_id_t m_fileid{};
    level_t m_level{};
    toolpex::ref_count m_ref;
};

class file_guard
{
public:
    constexpr file_guard();

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
