#ifndef FRENZYKV_ENV_H
#define FRENZYKV_ENV_H

#include <filesystem>
#include <memory>
#include <cstdint>
#include <chrono>
#include <system_error>

#include "toolpex/move_only.h"

#include "koios/task.h"

#include "frenzykv/writable.h"
#include "frenzykv/readable.h"
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
    virtual ::std::unique_ptr<random_readable> get_ramdom_readable(const ::std::filesystem::path& p) = 0;
    virtual ::std::unique_ptr<seq_writable>    get_seq_writable(const ::std::filesystem::path& p) = 0;
    virtual koios::task<> delete_file(const ::std::filesystem::path& p) = 0;
    virtual koios::task<> delete_dir(const ::std::filesystem::path& p) = 0;
    virtual koios::task<> move_file(const ::std::filesystem::path& from, const ::std::filesystem::path& to) = 0;
    virtual koios::task<> rename_file(const ::std::filesystem::path& p, const ::std::filesystem::path& newname) = 0;
    virtual koios::task<> sleep_for(::std::chrono::milliseconds ms) = 0;
    virtual koios::task<> sleep_until(::std::chrono::system_clock::time_point tp) = 0;
    virtual ::std::filesystem::path current_directory() const = 0;
    virtual ::std::error_code change_current_directroy(const ::std::filesystem::path& p) = 0;

    static ::std::unique_ptr<env> make_default_env(const options& opt);
};

::std::error_code recreate_dirs_if_non_exists();

class env_exception : public koios::exception
{
public:
    using koios::exception::exception;
};

}

#endif
