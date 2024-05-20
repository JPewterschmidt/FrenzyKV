#ifndef FRENZYKV_UTIL_FILE_GUARD_H
#define FRENZYKV_UTIL_FILE_GUARD_H

#include <filesystem>

#include "toolpex/ref_count.h"
#include "toolpex/assert.h"

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
        toolpex_assert(other.approx_ref_count() == 0);
    }

    file_rep& operator=(file_rep&& other) noexcept
    {
        toolpex_assert(approx_ref_count() == 0);
        toolpex_assert(other.approx_ref_count() == 0);

        m_level = other.m_level;
        m_fileid = ::std::move(other.m_fileid);
        m_name = ::std::move(other.m_name);

        return *this;
    }

    auto ref() { return m_ref++; }
    auto deref() { return m_ref--; }
    file_id_t file_id() const noexcept { return m_fileid; }
    level_t level() const noexcept { return m_level; }
    const ::std::string& name() const noexcept { return m_name; }
    ::std::filesystem::file_time_type last_write_time() const;
    uintmax_t file_size() const;
    
    bool valid() const noexcept { return !m_name.empty(); }
    operator file_id_t() const noexcept { return file_id(); }
    operator level_t() const noexcept { return level(); }
    operator ::std::string_view() const noexcept { return name(); }

    koios::task<::std::unique_ptr<random_readable>> open_read(env* e) const;
    koios::task<::std::unique_ptr<seq_writable>> open_write(env* e) const;

    bool operator==(const file_rep& other) const noexcept
    {
        if (this == &other) return true;
        toolpex_assert(file_id() != other.file_id());
        return false;
    }

    bool operator!=(const file_rep& other) const noexcept
    {
        return !(*this == other);
    }

    bool operator<(const file_rep& other) const noexcept
    {
        if (level() < other.level()) return true;
        else if (level() > other.level()) return false;
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

    file_guard(file_rep* rep) noexcept : m_rep{ rep } { m_rep->ref(); }
    file_guard(file_rep& rep) noexcept : m_rep{ &rep } { m_rep->ref(); }

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
        toolpex_assert(m_rep);
        return m_rep->file_id();
    }

    level_t level() const noexcept 
    {
        toolpex_assert(m_rep);
        return m_rep->level();
    }

    auto& rep() noexcept { toolpex_assert(m_rep); return *m_rep; }
    const auto& rep() const noexcept { toolpex_assert(m_rep); return *m_rep; }
    const ::std::string& name() const noexcept { toolpex_assert(m_rep); return m_rep->name(); }

    bool valid() const noexcept { return !!m_rep; }
    operator file_id_t() const noexcept { return file_id(); }
    operator level_t() const noexcept { return level(); }
    operator ::std::string_view() const { return name(); }
    operator ::std::filesystem::path() const { return name(); }
    auto last_write_time() const { toolpex_assert(valid()); return m_rep->last_write_time(); }
    auto file_size() const { toolpex_assert(valid()); return m_rep->file_size(); }

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

    koios::task<::std::unique_ptr<random_readable>> open_read(env* e) const
    {
        toolpex_assert(m_rep);
        return m_rep->open_read(e);
    }

    koios::task<::std::unique_ptr<seq_writable>> open_write(env* e) const
    {
        toolpex_assert(m_rep);
        return m_rep->open_write(e);
    }

public:
    template<::std::size_t Level>
    static bool with_level(const file_guard& fg) noexcept
    {
        return fg.level() == Level;
    }

    static auto with_level_predicator(level_t l) noexcept
    {
        return [=](const file_guard& fg) noexcept {
            return fg.level() == l;
        };
    }

    static bool have_same_level(const file_guard& lhs, const file_guard& rhs) noexcept
    {
        return lhs.level() == rhs.level();
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
