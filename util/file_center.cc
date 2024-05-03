#include <filesystem>
#include <cassert>

#include "frenzykv/env.h"
#include "frenzykv/util/file_center.h"

namespace fs = ::std::filesystem;
namespace r = ::std::ranges;
namespace rv = r::views;
using namespace ::std::string_literals;
using namespace ::std::string_view_literals;

namespace frenzykv
{

::std::string name_a_sst(level_t l, const file_id_t& id)
{
    return ::std::format("frzkv#{}#{}#.frzkvsst", l, id.to_string());
}

bool is_sst_name(const ::std::string& name)
{
    return name.starts_with("frzkv#") && name.ends_with("#.frzkvsst");   
}

::std::optional<::std::pair<level_t, file_id_t>>
retrive_level_and_id_from_sst_name(const ::std::string& name)
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

    result.emplace(::std::stoi(*firsti), file_id_t(*secondi));

    return result;
}

koios::task<> file_center::load_files()
{
    auto lk = co_await m_mutex.acquire();
    // Go through all the files.
    for (const auto& dir_entry : fs::directory_iterator{ sstables_path() })
    {
        auto name = dir_entry.path().filename().string();
        assert(!is_sst_name(name));

        auto level_and_id_opt = retrive_level_and_id_from_sst_name(name);
        m_name_rep[name] = ::std::make_unique<file_rep>(
            level_and_id_opt->first, level_and_id_opt->second, name
        );
    }

    co_return;
}

koios::task<::std::vector<file_guard>>
file_center::get_file_guards(::std::ranges::range auto const& names)
{
    ::std::vector<file_guard> result;
    auto lk = co_await m_mutex.acquire_shared();

    for (const auto& name : names)
    {
        result.emplace_back(m_name_rep.at(name).get());
    }

    co_return result;
}

} // namespace frenzykv
