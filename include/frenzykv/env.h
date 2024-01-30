#ifndef FRENZYKV_ENV_H
#define FRENZYKV_ENV_H

#include <filesystem>
#include <memory>
#include <cstdint>
#include <chrono>

#include "toolpex/move_only.h"

#include "koios/task.h"

#include "frenzykv/writable.h"
#include "frenzykv/readable.h"

namespace frenzykv
{

class env : public toolpex::move_only
{
public:
    env() = default;
    virtual ~env() noexcept {}
    virtual koios::task<::std::unique_ptr<seq_readable>>    get_seq_readable(const ::std::filesystem::path& p) = 0;
    virtual koios::task<::std::unique_ptr<random_readable>> get_ramdom_readable(const ::std::filesystem::path& p) = 0;
    virtual koios::task<::std::unique_ptr<seq_writable>>    get_seq_writable(const ::std::filesystem::path& p) = 0;
    virtual koios::task<> delete_file(const ::std::filesystem::path& p) = 0;
    virtual koios::task<> delete_dir(const ::std::filesystem::path& p) = 0;
    virtual koios::task<> move_file(const ::std::filesystem::path& from, const ::std::filesystem::path& to) = 0;
    virtual koios::task<> rename_file(const ::std::filesystem::path& p, const ::std::filesystem::path& newname) = 0;
    virtual koios::task<> sleep_for(::std::chrono::milliseconds ms) = 0;
    virtual koios::task<> sleep_until(::std::chrono::system_clock::time_point tp) = 0;
    // TODO filelock

    static env* default_env();
};

class env_exception : public koios::exception
{
public:
    using koios::exception::exception;
};

}

#endif
