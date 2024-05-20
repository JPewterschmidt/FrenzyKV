#include "toolpex/assert.h"
#include "koios/this_task.h"

#include "frenzykv/env.h"
#include "frenzykv/util/file_guard.h"

namespace fs = ::std::filesystem;

namespace frenzykv
{

uintmax_t file_rep::file_size() const
{
    return fs::file_size(sstables_path()/name());
}

koios::task<::std::unique_ptr<random_readable>>
file_rep::open_read(env* e) const
{
    auto ret = e->get_random_readable(sstables_path()/name());
    if (ret->file_size() == 0)
    {
        co_await koios::this_task::yield();
    }
    toolpex_assert(ret->file_size() != 0);
    co_return ret;
}

koios::task<::std::unique_ptr<seq_writable>>
file_rep::open_write(env* e) const
{
    co_return e->get_seq_writable(sstables_path()/name());
}

::std::filesystem::file_time_type 
file_rep::last_write_time() const
{
    return fs::last_write_time(sstables_path()/name());
}

} // namespace frenzykv
