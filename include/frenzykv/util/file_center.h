#ifndef FRENZYKV_UTIL_FILE_CENTER_H
#define FRENZYKV_UTIL_FILE_CENTER_H

#include <map>
#include <utility>
#include <string>
#include <memory>
#include <optional>

#include "toolpex/move_only.h"

#include "koios/task.h"
#include "koios/coroutine_shared_mutex.h"

#include "frenzykv/util/file_guard.h"
#include "frenzykv/types.h"
#include "frenzykv/kvdb_deps.h"

namespace frenzykv
{

::std::string name_a_sst(level_t l, const file_id_t& id);
::std::string name_a_sst(level_t l);

bool is_sst_name(const ::std::string& name);

::std::optional<::std::pair<level_t, file_id_t>>
retrive_level_and_id_from_sst_name(const ::std::string& name);

class file_center : public toolpex::move_only
{
public:
    file_center(const kvdb_deps& deps) noexcept
        : m_deps{ &deps }
    {
    }

    const kvdb_deps& deps() const noexcept { return *m_deps; }
    koios::task<> load_files();

    koios::task<::std::vector<file_guard>>
    get_file_guards(::std::ranges::range auto const& names);

    koios::task<file_guard> get_file(const ::std::string& name);

private:
    const kvdb_deps* m_deps;
    koios::shared_mutex m_mutex;
    ::std::map<::std::string, ::std::unique_ptr<file_rep>> m_name_rep;
};

} // namespace frenzykv

#endif
