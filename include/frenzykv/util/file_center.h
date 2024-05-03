#ifndef FRENZYKV_UTIL_FILE_CENTER_H
#define FRENZYKV_UTIL_FILE_CENTER_H

#include <map>
#include <utility>
#include <string>
#include <memory>
#include <optional>

#include "toolpex/move_only.h"

#include "koios/task.h"

#include "frenzykv/file_guard.h"
#include "frenzykv/types.h"

namespace frenzykv
{

::std::string name_a_sst(level_t l, const file_id_t& id);
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

    koios::task<> load_files();

private:
    const kvdb_deps& m_deps;
    ::std::map<::std::string, ::std::unique_ptr<file_rep>> m_name_rep;
};

} // namespace frenzykv

#endif
