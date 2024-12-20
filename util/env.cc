// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "toolpex/bit_mask.h"

#include "frenzykv/env.h"

#include "frenzykv/io/iouring_readable.h"
#include "frenzykv/io/iouring_writable.h"
#include "frenzykv/io/in_mem_rw.h"

#include "koios/iouring_awaitables.h"
#include "koios/this_task.h"

#include <unistd.h>
#include <cerrno>
#include <system_error>
#include <mutex>

#include "spdlog/spdlog.h"

using namespace koios;
namespace fs = ::std::filesystem;

namespace
{
    static fs::path root_path;
}

namespace frenzykv
{

class posix_uring_env : public env
{
private:
    const options* m_opt;

public:
    posix_uring_env(const options& opt) noexcept : m_opt{ &opt } {}

    ::std::unique_ptr<seq_readable>
    get_seq_readable(const fs::path& p) override
	{
        return ::std::make_unique<iouring_readable>(p, *m_opt);
	}

    ::std::unique_ptr<random_readable>
    get_random_readable(const fs::path& p) override
	{
        return ::std::make_unique<iouring_readable>(p, *m_opt);
	}

    ::std::unique_ptr<seq_writable>
    get_seq_writable(const fs::path& p) override
	{
        return ::std::make_unique<iouring_writable>(p, *m_opt);
	}

    ::std::unique_ptr<seq_writable>
    get_truncate_seq_writable(const ::std::filesystem::path& p) override
    {
        return ::std::make_unique<iouring_writable>(
            p, *m_opt, 
            iouring_writable::default_create_mode(), 
            O_TRUNC
        );
    }

    koios::task<>
	delete_file(const fs::path& p) override
	{
        co_await uring::unlink(p);
	}

    koios::task<>
	delete_dir(const fs::path& p) override
	{
        fs::remove_all(p);
        co_return;
	}

    koios::task<>
	move_file(const fs::path& from, const fs::path& to) override
	{
        return rename_file(from, to);
	}

    koios::task<>
	rename_file(const fs::path& p, 
                const fs::path& newname) override
	{
        if (p == newname) [[unlikely]] co_return;

        auto rename_ret = co_await uring::rename(p, newname);
        if (auto ec = rename_ret.error_code(); ec)
        {
            throw env_exception{ec};
        }
	}

    koios::task<> 
    sleep_for(::std::chrono::milliseconds ms) override
    {
        co_await this_task::sleep_for(ms);
    }

    koios::task<> 
    sleep_until(::std::chrono::system_clock::time_point tp) override
    {
        co_await this_task::sleep_until(tp);
    }

    fs::path 
    current_directory() const override
    {
        return fs::current_path();
    }
};

::std::unique_ptr<env> env::make_default_env(const options& opt)
{
    auto result = ::std::make_unique<posix_uring_env>(opt);
    result->m_root_path = opt.root_path;
    return result;
}

namespace fs = fs;

fs::path env::sstables_path()
{
    return this->m_root_path/"sstable";
}

fs::path env::write_ahead_log_path()
{
    return this->m_root_path/"write_ahead_log";
}

fs::path env::system_log_path()
{
    return this->m_root_path/"system_log";
}

fs::path env::config_path()
{
    return this->m_root_path/"config";
}

::std::filesystem::path env::version_path()
{
    return this->m_root_path/"version";
}

fs::directory_entry env::sstables_dir(::std::error_code& ec)
{
    return { sstables_path(), ec };
}

fs::directory_entry env::write_ahead_log_dir(::std::error_code& ec)
{
    return { write_ahead_log_path(), ec };
}

fs::directory_entry env::system_log_dir(::std::error_code& ec)
{
    return { system_log_path(), ec };
}

fs::directory_entry env::config_dir(::std::error_code& ec)
{
    return { config_path(), ec };
}

::std::filesystem::directory_entry env::version_dir(::std::error_code& ec)
{
    return { version_path(), ec };
}

fs::directory_entry env::sstables_dir()
{
	::std::error_code ec;
    fs::directory_entry result = sstables_dir(ec);
    if (ec) throw koios::exception(ec);

    return result;
}

fs::directory_entry env::write_ahead_log_dir()
{
	::std::error_code ec;
    fs::directory_entry result = write_ahead_log_dir(ec);
    if (ec) throw koios::exception(ec);

    return result;
}

fs::directory_entry env::system_log_dir()
{
	::std::error_code ec;
    fs::directory_entry result = system_log_dir(ec);
    if (ec) throw koios::exception(ec);

    return result;
}

fs::directory_entry env::config_dir()
{
	::std::error_code ec;
    fs::directory_entry result = config_dir(ec);
    if (ec) throw koios::exception(ec);

    return result;
}

fs::directory_entry env::version_dir()
{
	::std::error_code ec;
    fs::directory_entry result = version_dir(ec);
    if (ec) throw koios::exception(ec);

    return result;
}

::std::error_code env::recreate_dirs_if_non_exists()
{
    // assume that we are already in the working directory
    ::std::error_code result{};
    const auto cur_dir = fs::current_path(result);
    if (result) return result;
    
    if (!toolpex::bit_mask{fs::status(cur_dir).permissions()}
            .contains(fs::perms::owner_read | fs::perms::owner_write))
    {
        return { EPERM, ::std::system_category() };
    }

    result = {};

    if (!fs::exists(sstables_path(), result) && !fs::create_directory(sstables_path(), result)) return result;
    result = {};
    if (!fs::exists(write_ahead_log_path(), result) && !fs::create_directory(write_ahead_log_path(), result)) return result;
    result = {};
    if (!fs::exists(system_log_path(), result) && !fs::create_directory(system_log_path(), result)) return result;
    result = {};
    if (!fs::exists(config_path(), result) && !fs::create_directory(config_path(), result)) return result;
    result = {};
    if (!fs::exists(version_path(), result) && !fs::create_directory(version_path(), result)) return result;

    return result;
}

}
