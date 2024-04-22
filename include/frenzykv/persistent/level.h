#ifndef FRENZYKV_PERSISTENT_LEVEL_H
#define FRENZYKV_PERSISTENT_LEVEL_H

#include <string>
#include <unordered_map>
#include <queue>
#include <atomic>

#undef BLOCK_SIZE
#include "concurrentqueue/concurrentqueue.h"

#include "koios/task.h"
#include "koios/coroutine_shared_mutex.h"

#include "frenzykv/kvdb_deps.h"
#include "frenzykv/types.h"
#include "frenzykv/io/writable.h"
#include "frenzykv/io/readable.h"

namespace frenzykv
{

using file_id_t = uint32_t;
using level_t = size_t;

class level
{
public:
    level(const kvdb_deps& deps);

    koios::task<file_id_t> create_file(level_t level);
    koios::task<> delete_file(level_t level, file_id_t id);

    koios::task<::std::unique_ptr<seq_writable>> open_write(level_t level, file_id_t id);
    koios::task<::std::unique_ptr<random_readable>> open_read(level_t level, file_id_t id);

    koios::task<> start() noexcept;
    koios::task<> finish() noexcept;

private:
    koios::task<file_id_t> allocate_file_id();
    bool working() const noexcept;

private:
    const kvdb_deps* m_deps{};
    ::std::atomic_int m_working{};
    ::std::unordered_map<file_id_t, ::std::string> m_id_name;
    ::std::atomic<file_id_t> m_latest_unused_id{};
    ::std::queue<file_id_t> m_id_recycled; 

    mutable koios::shared_mutex m_mutex;
};

} // namespace frenzykv

#endif
