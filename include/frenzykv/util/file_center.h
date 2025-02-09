// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_UTIL_FILE_CENTER_H
#define FRENZYKV_UTIL_FILE_CENTER_H

#include <map>
#include <utility>
#include <string>
#include <memory>
#include <optional>
#include <vector>
#include <list>
#include <string_view>

#include "toolpex/move_only.h"

#include "koios/task.h"
#include "koios/coroutine_mutex.h"

#include "frenzykv/util/file_guard.h"
#include "frenzykv/types.h"
#include "frenzykv/kvdb_deps.h"

namespace frenzykv
{

inline consteval size_t sst_name_length() noexcept 
{ 
    using namespace ::std::string_view_literals;
    return "frzkv#****0#030f57fe-2405-47e9-a300-588812af06c9#.frzkvsst"sv.size();
}

::std::string name_a_sst(level_t l, const file_id_t& id);
::std::string name_a_sst(level_t l);

bool is_sst_name(::std::string_view name);

::std::optional<::std::pair<level_t, file_id_t>>
retrive_level_and_id_from_sst_name(::std::string_view name);

class file_center : public toolpex::move_only
{
public:
    file_center(const kvdb_deps& deps) noexcept;

    const kvdb_deps& deps() const noexcept { return *m_deps; }
    koios::lazy_task<> load_files();

    koios::task<::std::vector<file_guard>>
    get_file_guards(const ::std::vector<::std::string>& names);

    koios::task<file_guard> get_file(const ::std::string& name);

    koios::task<> GC();

private:
    const kvdb_deps* m_deps;
    //koios::shared_mutex m_mutex;
    koios::mutex m_mutex;

    // The reason why we need use a unique_ptr wrapper
    // is because we want to keep the address of those reps
    // stable. cause the `version_guard`s refer them by addresses.
    ::std::list<::std::unique_ptr<file_rep>> m_reps;
    ::std::map<::std::string, file_rep*> m_name_rep;
};

} // namespace frenzykv

#endif
