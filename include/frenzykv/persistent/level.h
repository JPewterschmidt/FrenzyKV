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
#include "frenzykv/util/file_guard.h"
#include "frenzykv/util/uuid.h"

namespace frenzykv
{

::std::string name_a_sst(level_t l, const file_id_t& id);

::std::optional<::std::pair<level_t, file_id_t>>
retrive_level_and_id_from_sst_name(const ::std::string& name);

class level
{
public:
    level(const kvdb_deps& deps);

    koios::task<::std::pair<file_guard, ::std::unique_ptr<seq_writable>>> 
    create_file(level_t level);

    koios::task<::std::unique_ptr<seq_writable>> open_write(const file_guard& guard);
    koios::task<::std::unique_ptr<random_readable>> open_read(const file_guard& guard);

    koios::task<> start() noexcept;
    koios::task<> finish() noexcept;

    /*! \brief Return the max number of SSTable of specific level
     *  \retval 0   There's no exact restriction of file number.
     *  \retval !=0 the max number of files.
     */
    koios::task<size_t> allowed_file_number(level_t l) const noexcept;

    /*! \brief Return the max bytes size each SSTable of specific level
     *  \retval 0   There's no exact restriction of file number.
     *  \retval !=0 the max size of a sstable.
     */
    koios::task<size_t> allowed_file_size(level_t l) const noexcept;
    
    koios::task<bool> need_to_comapct(level_t l) const noexcept;

    koios::task<size_t> actual_file_number(level_t l) const noexcept;

    koios::task<::std::vector<file_guard>> level_file_guards(level_t l) noexcept;

    koios::task<size_t> level_number() const noexcept;

    koios::task<file_guard> oldest_file(const ::std::vector<file_guard>& files);
    koios::task<file_guard> oldest_file(level_t l);

    /*  \brief Garbage Collect Function
     *
     *  This function will delete all the out-dataed files, 
     *  which were not been refered by any file_guard.
     *  A version should refer several SSTs.
     *
     *  \attention So before version_center been initialized, do not call this function
     *             Or you will delete all those files.
     */
    koios::task<> GC();

private:
    koios::task<> delete_file(const file_rep& rep);
    bool working() const noexcept;

    koios::task<bool> level_contains(const file_guard& guard) const
    {
        return level_contains(guard.rep());
    }

    koios::task<bool> level_contains(const file_rep& rep) const;

private:
    const kvdb_deps* m_deps{};
    ::std::atomic_int m_working{};
    ::std::map<file_id_t, ::std::string> m_id_name;

    ::std::vector<::std::vector<::std::unique_ptr<file_rep>>> m_levels_file_rep;

    mutable koios::shared_mutex m_mutex;
};

} // namespace frenzykv

#endif
