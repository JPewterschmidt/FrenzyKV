// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_UTIL_FILE_GUARD_H
#define FRENZYKV_UTIL_FILE_GUARD_H

#include <filesystem>

#include "toolpex/ref_count.h"
#include "toolpex/assert.h"

#include "koios/task.h"

#include "frenzykv/types.h"
#include "frenzykv/io/readable.h"
#include "frenzykv/io/writable.h"
#include "frenzykv/util/file_rep.h"

namespace frenzykv
{

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

    koios::task<::std::unique_ptr<random_readable>> open_read() const
    {
        toolpex_assert(m_rep);
        return m_rep->open_read();
    }

    koios::task<::std::unique_ptr<seq_writable>> open_write() const
    {
        toolpex_assert(m_rep);
        return m_rep->open_write();
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
