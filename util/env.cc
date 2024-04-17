#include "toolpex/bit_mask.h"

#include "frenzykv/env.h"

#include "frenzykv/iouring_readable.h"
#include "frenzykv/iouring_writable.h"
#include "frenzykv/in_mem_rw.h"
#include "frenzykv/iouring_readable.h"
#include "frenzykv/iouring_writable.h"

#include "koios/iouring_awaitables.h"
#include "koios/this_task.h"

#include <unistd.h>
#include <cerrno>
#include <system_error>

using namespace koios;

namespace frenzykv
{

class posix_uring_env : public env
{
private:
    const options* m_opt;

public:
    posix_uring_env(const options& opt) noexcept : m_opt{ &opt } {}

    ::std::unique_ptr<seq_readable>
    get_seq_readable(const ::std::filesystem::path& p) override
	{
        return ::std::make_unique<iouring_readable>(p, *m_opt);
	}

    ::std::unique_ptr<random_readable>
    get_ramdom_readable(const ::std::filesystem::path& p) override
	{
        return ::std::make_unique<iouring_readable>(p, *m_opt);
	}

    ::std::unique_ptr<seq_writable>
    get_seq_writable(const ::std::filesystem::path& p) override
	{
        return ::std::make_unique<iouring_writable>(p, *m_opt);
	}

    koios::task<>
	delete_file(const ::std::filesystem::path& p) override
	{
        co_await uring::unlink(p);
	}

    koios::task<>
	delete_dir(const ::std::filesystem::path& p) override
	{
        ::std::filesystem::remove_all(p);
        co_return;
	}

    koios::task<>
	move_file(const ::std::filesystem::path& from, const ::std::filesystem::path& to) override
	{
        return rename_file(from, to);
	}

    koios::task<>
	rename_file(const ::std::filesystem::path& p, 
                const ::std::filesystem::path& newname) override
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

    ::std::filesystem::path 
    current_directory() const override
    {
        return ::std::filesystem::current_path();
    }

    ::std::error_code 
    change_current_directroy(const ::std::filesystem::path& p) override
    {
        ::std::error_code result{};
        ::std::filesystem::current_path(p, result);
        return result;
    }
};

::std::unique_ptr<env> env::make_default_env(const options& opt)
{
    static auto result = [&]{ 
        posix_uring_env env(opt);
        env.change_current_directroy(opt.root_path);
        return env;
    }();
    return ::std::make_unique<posix_uring_env>(::std::move(result));
}

namespace fs = ::std::filesystem;

::std::filesystem::path sstables_path()
{
    return fs::current_path()/"sstable";
}

::std::filesystem::path prewrite_log_path()
{
    return fs::current_path()/"db_prewrite_log";
}

::std::filesystem::path system_log_path()
{
    return fs::current_path()/"system_log";
}

::std::filesystem::path config_path()
{
    return fs::current_path()/"config";
}

::std::filesystem::directory_entry sstables_dir(::std::error_code& ec)
{
    return { sstables_path(), ec };
}

::std::filesystem::directory_entry prewrite_log_dir(::std::error_code& ec)
{
    return { prewrite_log_path(), ec };
}

::std::filesystem::directory_entry system_log_dir(::std::error_code& ec)
{
    return { system_log_path(), ec };
}

::std::filesystem::directory_entry config_dir(::std::error_code& ec)
{
    return { config_path(), ec };
}

::std::error_code recreate_dirs_if_non_exists()
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
    if (!fs::create_directory(sstables_path(), result)) return result;
    if (!fs::create_directory(prewrite_log_path(), result)) return result;
    if (!fs::create_directory(system_log_path(), result)) return result;
    if (!fs::create_directory(config_path(), result)) return result;

    return result;
}

}
