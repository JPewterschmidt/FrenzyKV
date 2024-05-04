#ifndef FRENZYKV_UTIL_FILE_GUARD_H
#define FRENZYKV_UTIL_FILE_GUARD_H

#include <cassert>

#include "toolpex/ref_count.h"

#include "koios/task.h"

#include "frenzykv/types.h"
#include "frenzykv/env.h"
#include "frenzykv/io/readable.h"
#include "frenzykv/io/writable.h"

namespace frenzykv
{

class file_rep
{
public:
    constexpr file_rep() noexcept = default;

    file_rep(level_t l, file_id_t id, ::std::string name) noexcept
        : m_level{ l }, m_fileid{ id }, m_name{ ::std::move(name) }
    {
    }

    auto approx_ref_count() const noexcept { return m_ref.load(::std::memory_order_relaxed); }

    file_rep(file_rep&& other) noexcept
        : m_level{ other.m_level }, 
          m_fileid{ ::std::move(other.m_fileid) }, 
          m_name{ ::std::move(other.m_name) }
    {
        assert(other.approx_ref_count() == 0);
    }

    file_rep& operator=(file_rep&& other) noexcept
    {
        assert(approx_ref_count() == 0);
        assert(other.approx_ref_count() == 0);

        m_level = other.m_level;
        m_fileid = ::std::move(other.m_fileid);
        m_name = ::std::move(other.m_name);

        return *this;
    }

    auto ref() { return m_ref++; }
    auto deref() { return m_ref--; }
    file_id_t file_id() const noexcept { return m_fileid; }
    level_t level() const noexcept { return m_level; }
    ::std::string_view name() const noexcept { return m_name; }
    
    operator file_id_t() const noexcept { return file_id(); }
    operator level_t() const noexcept { return level(); }
    operator ::std::string_view() const noexcept { return name(); }

    ::std::unique_ptr<random_readable> open_read(env* e) const;
    ::std::unique_ptr<seq_writable> open_write(env* e) const;

    bool operator==(const file_rep& other) const noexcept
    {
        if (this == &other) return true;
        assert(file_id() != other.file_id());
        return false;
    }

    bool operator!=(const file_rep& other) const noexcept
    {
        return !(*this == other);
    }

    bool operator<(const file_rep& other) const noexcept
    {
        // For set difference
        return file_id() < other.file_id();
    }

private:
    level_t m_level{};
    file_id_t m_fileid{};
    toolpex::ref_count m_ref;
    ::std::string m_name{};
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
        if (m_rep) m_rep->ref();
    }

    file_guard& operator=(const file_guard& other) noexcept
    {
        release();
        m_rep = other.m_rep;
        if (m_rep) m_rep->ref();
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
    const ::std::string_view name() const noexcept { assert(m_rep); return m_rep->name(); }

    operator file_id_t() const noexcept { return file_id(); }
    operator level_t() const noexcept { return level(); }
    operator ::std::string_view() const { return name(); }

    bool operator==(const file_guard& other) const noexcept
    {
        return m_rep == other.m_rep;
    }

    bool operator!=(const file_guard& other) const noexcept
    {
        return !(*this == other);
    }

    bool operator<(const file_guard& other) const noexcept
    {
        return (*m_rep < *other.m_rep);
    }

    ::std::unique_ptr<random_readable> open_read(env* e) const
    {
        assert(m_rep);
        return m_rep->open_read(e);
    }

    ::std::unique_ptr<seq_writable> open_write(env* e) const
    {
        assert(m_rep);
        return m_rep->open_write(e);
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
    file_rep* m_rep{};
};

} // namespace frenzykv

#endif
