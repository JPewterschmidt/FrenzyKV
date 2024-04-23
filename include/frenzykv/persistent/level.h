#ifndef FRENZYKV_PERSISTENT_LEVEL_H
#define FRENZYKV_PERSISTENT_LEVEL_H

#include <string>
#include <unordered_map>
#include <queue>
#include <atomic>
#include <memory>
#include <utility>

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

    koios::task<::std::pair<file_id_t, ::std::unique_ptr<seq_writable>>> 
    create_file(level_t level);

    koios::task<> delete_file(level_t level, file_id_t id);

    koios::task<::std::unique_ptr<seq_writable>> open_write(level_t level, file_id_t id);
    koios::task<::std::unique_ptr<random_readable>> open_read(level_t level, file_id_t id);

    koios::task<> start() noexcept;
    koios::task<> finish() noexcept;

    /*! \brief Return the max number of SSTable of specific level
     *  \retval 0   There's no exact restriction of file number.
     *  \retval !=0 the max number of files.
     */
    size_t allowed_file_number(level_t l) const noexcept;

    /*! \brief Return the max bytes size each SSTable of specific level
     *  \retval 0   There's no exact restriction of file number.
     *  \retval !=0 the max size of a sstable.
     */
    size_t allowed_file_size(level_t l) const noexcept;
    
    bool need_to_comapct(level_t l) const noexcept;

    size_t actual_file_number(level_t l) const noexcept;

    const ::std::vector<file_id_t>& level_file_ids(level_t l) const noexcept;

    size_t level_number() const noexcept;

    file_id_t oldest_file(const ::std::vector<file_id_t>& files) const;
    file_id_t oldest_file(level_t l) const;

private:
    koios::task<file_id_t> allocate_file_id();
    bool working() const noexcept;

private:
    const kvdb_deps* m_deps{};
    ::std::atomic_int m_working{};
    ::std::unordered_map<file_id_t, ::std::string> m_id_name;
    ::std::atomic<file_id_t> m_latest_unused_id{};
    ::std::queue<file_id_t> m_id_recycled;

    ::std::vector<::std::vector<file_id_t>> m_levels_file_id;

    mutable koios::shared_mutex m_mutex;
};

} // namespace frenzykv

#endif
