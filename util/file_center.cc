// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <filesystem>
#include <ranges>
#include <functional>
#include <memory>

#include "toolpex/assert.h"

#include "koios/iouring_awaitables.h"

#include "frenzykv/env.h"
#include "frenzykv/util/file_center.h"
#include "frenzykv/util/file_guard.h"

namespace fs = ::std::filesystem;
namespace r = ::std::ranges;
namespace rv = r::views;
using namespace ::std::string_literals;
using namespace ::std::string_view_literals;

[[maybe_unused]]
static bool operator<(const ::std::unique_ptr<frenzykv::file_rep>& lhs, const ::std::unique_ptr<frenzykv::file_rep>& rhs) noexcept
{
    toolpex_assert(!!lhs && !!rhs);
    return *lhs < *rhs;
}

namespace frenzykv
{

::std::string name_a_sst(level_t l, const file_id_t& id)
{
    return ::std::format("frzkv#{:*>5}#{}#.frzkvsst", l, id.to_string());
}

::std::string name_a_sst(level_t l)
{
    return name_a_sst(l, file_id_t{});
}

bool is_sst_name(::std::string_view name)
{
    return name.starts_with("frzkv#") && name.ends_with("#.frzkvsst");   
}

::std::optional<::std::pair<level_t, file_id_t>>
retrive_level_and_id_from_sst_name(::std::string_view name)
{
    ::std::optional<::std::pair<level_t, file_id_t>> result;
    if (!is_sst_name(name))
        return result;

    auto data_view = name 
        | rv::split("#"s)
        | rv::transform([](auto&& str) { return ::std::string(str.begin(), str.end()); })
        | rv::drop(1)
        | rv::take(2)
        ;

    auto firsti = begin(data_view);
    auto secondi = next(firsti);

    auto first_str = *firsti;

    result.emplace(::std::stoi(first_str.substr(first_str.find_first_not_of('*'))), file_id_t(*secondi));

    return result;
}

file_center::file_center(const kvdb_deps& deps) noexcept
    : m_deps{ &deps }
{
}

koios::lazy_task<> file_center::load_files()
{
    auto env = m_deps->env();
    auto lk = co_await m_mutex.acquire();
    // Go through all the files.
    for (const auto& dir_entry : fs::directory_iterator{ env->sstables_path() })
    {
        if (dir_entry.file_size() == 0) 
        {
            co_await koios::uring::unlink(dir_entry.path());
            continue;
        }
        auto name = dir_entry.path().filename().string();
        toolpex_assert(is_sst_name(name));

        auto level_and_id_opt = retrive_level_and_id_from_sst_name(name);
        auto& sp = m_reps.emplace_back(::std::make_unique<file_rep>(this, level_and_id_opt->first, level_and_id_opt->second, name));
        m_name_rep[name] = sp.get();
    }

    co_return;
}

koios::task<::std::vector<file_guard>>
file_center::get_file_guards(const ::std::vector<::std::string>& names)
{
    ::std::vector<file_guard> result;
    auto lk = co_await m_mutex.acquire();

    for (const auto& name : names)
    {
        toolpex_assert(is_sst_name(name));
        result.emplace_back(m_name_rep.at(name));
    }

    co_return result;
}

koios::task<file_guard> file_center::get_file(const ::std::string& name)
{
    toolpex_assert(is_sst_name(name));
    auto lk = co_await m_mutex.acquire();
    if (m_name_rep.contains(name))
    {
        co_return *m_name_rep[name];
    }
    auto level_and_id_opt = retrive_level_and_id_from_sst_name(name);
    auto& sp = m_reps.emplace_back(
        ::std::make_unique<file_rep>(
            this, level_and_id_opt->first, level_and_id_opt->second, name
        )
    );
    auto insert_ret = m_name_rep.insert({ name, sp.get() });
    toolpex_assert(insert_ret.second); // May triggered
    co_return *((*(insert_ret.first)).second);
}

koios::task<> file_center::GC()
{
    auto lk = co_await m_mutex.acquire();
    auto removing = r::partition(m_reps, [](auto&& rep){ return rep->approx_ref_count() != 0; });
    auto names_removing = removing | rv::transform([](auto&& r) { toolpex_assert(r->approx_ref_count() == 0); return r->name(); });
    auto env = m_deps->env();
    for (auto name : names_removing)
    {
        m_name_rep.erase(::std::string(name));
        co_await koios::uring::unlink(env->sstables_path()/name);
    }
    m_reps.erase(begin(removing), end(removing));

    co_return;
}

} // namespace frenzykv
