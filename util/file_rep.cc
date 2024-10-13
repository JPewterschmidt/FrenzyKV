// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "toolpex/assert.h"
#include "koios/this_task.h"

#include "frenzykv/env.h"
#include "frenzykv/util/file_rep.h"
#include "frenzykv/util/file_center.h"

namespace fs = ::std::filesystem;

namespace frenzykv
{

file_rep::file_rep(file_center* center, level_t l, file_id_t id, ::std::string name) noexcept
    : m_env{ center->deps().env().get() }, 
      m_level{ l }, 
      m_fileid{ id }, 
      m_name{ ::std::move(name) }
{
}

uintmax_t file_rep::file_size() const
{
    return fs::file_size(m_env->sstables_path()/name());
}

koios::task<::std::unique_ptr<random_readable>>
file_rep::open_read() const
{
    auto ret = m_env->get_random_readable(m_env->sstables_path()/name());
    if (ret->file_size() == 0)
    {
        co_await koios::this_task::yield();
    }
    toolpex_assert(ret->file_size() != 0);
    co_return ret;
}

koios::task<::std::unique_ptr<seq_writable>>
file_rep::open_write() const
{
    co_return m_env->get_seq_writable(m_env->sstables_path()/name());
}

::std::filesystem::file_time_type 
file_rep::last_write_time() const
{
    return fs::last_write_time(m_env->sstables_path()/name());
}

} // namespace frenzykv
