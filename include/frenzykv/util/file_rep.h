// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_UTIL_FILE_REP_H
#define FRENZYKV_UTIL_FILE_REP_H

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

class file_center;

class file_rep
{
public:
    constexpr file_rep() noexcept = default;

    file_rep(file_center* center, level_t l, file_id_t id, ::std::string name) noexcept;

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

    koios::task<::std::unique_ptr<random_readable>> open_read() const;
    koios::task<::std::unique_ptr<seq_writable>> open_write() const;

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
    env* m_env;
    level_t m_level{};
    file_id_t m_fileid{};
    toolpex::ref_count m_ref;
    ::std::string m_name{};
};

} // namespace frenzykv

#endif
