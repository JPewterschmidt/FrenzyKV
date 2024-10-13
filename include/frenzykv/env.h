// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_ENV_H
#define FRENZYKV_ENV_H

#include <filesystem>
#include <memory>
#include <cstdint>
#include <chrono>
#include <system_error>

#include "toolpex/move_only.h"

#include "koios/task.h"

#include "frenzykv/io/writable.h"
#include "frenzykv/io/readable.h"
#include "frenzykv/options.h"

namespace frenzykv
{

class env : public toolpex::move_only
{
public:
    env() = default;
    env(env&&) noexcept = default;
    env& operator = (env&&) noexcept = default;
    virtual ~env() noexcept {}

    virtual ::std::unique_ptr<seq_readable>    get_seq_readable(const ::std::filesystem::path& p) = 0;
    virtual ::std::unique_ptr<random_readable> get_random_readable(const ::std::filesystem::path& p) = 0;
    virtual ::std::unique_ptr<seq_writable>    get_seq_writable(const ::std::filesystem::path& p) = 0;

    virtual ::std::unique_ptr<seq_writable>    get_truncate_seq_writable(const ::std::filesystem::path& p) = 0;

    virtual koios::task<> delete_file(const ::std::filesystem::path& p) = 0;
    virtual koios::task<> delete_dir(const ::std::filesystem::path& p) = 0;
    virtual koios::task<> move_file(const ::std::filesystem::path& from, const ::std::filesystem::path& to) = 0;
    virtual koios::task<> rename_file(const ::std::filesystem::path& p, const ::std::filesystem::path& newname) = 0;
    virtual koios::task<> sleep_for(::std::chrono::milliseconds ms) = 0;
    virtual koios::task<> sleep_until(::std::chrono::system_clock::time_point tp) = 0;
    virtual ::std::filesystem::path current_directory() const = 0;

    static ::std::unique_ptr<env> make_default_env(const options& opt);

    virtual ::std::error_code recreate_dirs_if_non_exists();
    virtual ::std::filesystem::directory_entry sstables_dir(::std::error_code& ec);
    virtual ::std::filesystem::directory_entry write_ahead_log_dir(::std::error_code& ec);
    virtual ::std::filesystem::directory_entry system_log_dir(::std::error_code& ec);
    virtual ::std::filesystem::directory_entry config_dir(::std::error_code& ec);
    virtual ::std::filesystem::directory_entry version_dir(::std::error_code& ec);
    virtual ::std::filesystem::directory_entry sstables_dir();
    virtual ::std::filesystem::directory_entry write_ahead_log_dir();
    virtual ::std::filesystem::directory_entry system_log_dir();
    virtual ::std::filesystem::directory_entry config_dir();
    virtual ::std::filesystem::directory_entry version_dir();
    virtual ::std::filesystem::path sstables_path();
    virtual ::std::filesystem::path write_ahead_log_path();
    virtual ::std::filesystem::path system_log_path();
    virtual ::std::filesystem::path config_path();
    virtual ::std::filesystem::path version_path();

private:
    ::std::filesystem::path m_root_path;
};

inline consteval ::std::string_view current_version_descriptor_name()
{
    return "current_version_descriptor";
}

class env_exception : public koios::exception
{
public:
    using koios::exception::exception;
};

}

#endif
