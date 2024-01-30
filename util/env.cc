#include "frenzykv/env.h"

#include "frenzykv/iouring_readable.h"
#include "frenzykv/iouring_writable.h"
#include "frenzykv/in_mem_rw.h"
#include "frenzykv/iouring_readable.h"
#include "frenzykv/iouring_writable.h"

#include "koios/iouring_awaitables.h"
#include "koios/iouring_rename_aw.h"
#include "koios/this_task.h"

using namespace koios;

namespace frenzykv
{

class posix_uring_env : public env
{
public:
    koios::task<::std::unique_ptr<seq_readable>> 
    get_seq_readable(const ::std::filesystem::path& p) override
	{
        co_return ::std::make_unique<iouring_readable>(p);
	}

    koios::task<::std::unique_ptr<random_readable>> 
    get_ramdom_readable(const ::std::filesystem::path& p) override
	{
        co_return ::std::make_unique<iouring_readable>(p);
	}

    koios::task<::std::unique_ptr<seq_writable>> 
    get_seq_writable(const ::std::filesystem::path& p) override
	{
        co_return ::std::make_unique<iouring_writable>(p);
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
};

env* env::default_env()
{
    static posix_uring_env result{};
    return &result;
}

}
