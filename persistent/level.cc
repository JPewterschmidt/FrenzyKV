#include <format>
#include <cassert>
#include <ranges>
#include <filesystem>
#include <algorithm>
#include <vector>

#include "koios/iouring_awaitables.h"

#include "frenzykv/env.h"
#include "frenzykv/persistent/level.h"

namespace frenzykv
{

namespace fs = ::std::filesystem;
namespace rv = ::std::ranges::views;
using namespace ::std::string_literals;
using namespace ::std::string_view_literals;

static ::std::string name_a_file(level_t l, file_id_t id)
{
    return ::std::format("frzkv-{}-{}-.frzkv", l, id);
}

static bool is_name_allcated_here(const ::std::string& name)
{
    return name.starts_with("frzkv-") && name.ends_with("-.frzkv");   
}

static 
::std::optional<::std::pair<level_t, file_id_t>>
retrive_level_and_id_from_name(const ::std::string& name)
{
    ::std::optional<::std::pair<level_t, file_id_t>> result;
    if (!is_name_allcated_here(name))
        return result;

    auto data_view = name 
        | rv::split("-"s)
        | rv::transform([](auto&& str) { return ::std::stoi(::std::string{ str.begin(), str.end() }); })
        | rv::drop(1)
        | rv::take(2)
        ;

    auto firsti = begin(data_view);
    auto secondi = next(firsti);

    result.emplace(*firsti, *secondi);

    return result;
}

level::level(const kvdb_deps& deps) 
   : m_deps{ &deps }, 
     m_levels_file_id(m_deps->opt()->max_level)
{
}

koios::task<file_id_t> level::allocate_file_id()
{
    assert(working());
    file_id_t result{};
    if (m_id_recycled.empty())
    {
        result = m_latest_unused_id.fetch_add(1);
    }
    else 
    {
        result = m_id_recycled.front();
        m_id_recycled.pop();
    }

    co_return result;
}

size_t level::allowed_file_number(level_t l)
{
    auto opt = m_deps->opt();
    const auto& number_vec = opt->level_file_number;
    if (l >= number_vec.size()) return 0;
    return number_vec[l];
}

size_t level::allowed_file_size(level_t l)
{
    auto opt = m_deps->opt();
    const auto& number_vec = opt->level_file_size;
    if (l >= number_vec.size()) return 0;
    return number_vec[l];
}

koios::task<::std::pair<file_id_t, ::std::unique_ptr<seq_writable>>> 
level::create_file(level_t level)
{
    assert(level < m_levels_file_id.size());
    assert(working());
    file_id_t id = co_await allocate_file_id();
    auto name = name_a_file(level, id);

    auto file = m_deps->env()->get_seq_writable(sstables_path()/name);

    m_id_name[id] = ::std::move(name);
    m_levels_file_id[level].push_back(id);
    
    co_return { id, ::std::move(file) };
}

koios::task<> level::delete_file(level_t level, file_id_t id)
{
    assert(working());
    auto lk = co_await m_mutex.acquire();
    co_await m_deps->env()->delete_file(sstables_path()/m_id_name.at(id));
    m_id_name.erase(id);
    ::std::erase(m_levels_file_id[level], id);

    // Recycle file id;
    m_id_recycled.push(id);
}

koios::task<> level::finish() noexcept
{
    int val = m_working.load();
    if (val == 2) co_return;
    else if (val == 0) 
        throw koios::exception{"level: you can not finish a unstarted level object"};

    auto lk = co_await m_mutex.acquire();
    
    m_working = false;
    co_return;
}

koios::task<> level::start() noexcept
{
    int val = m_working.load();
    if (val == 1) co_return;
    else if (val == 2) 
        throw koios::exception{"level: you can not start a finished level object"};

    auto lk = co_await m_mutex.acquire();

    ::std::vector<file_id_t> id_used;

    // Go through all the files.
    for (const auto& dir_entry : fs::directory_iterator{ sstables_path() })
    {
        auto name = dir_entry.path().filename().string();
        auto level_and_id_opt = retrive_level_and_id_from_name(name);
        if (!level_and_id_opt) 
            continue;
        m_id_name[level_and_id_opt->second] = name;

        m_levels_file_id[level_and_id_opt->first]
            .push_back(level_and_id_opt->second);

        id_used.push_back(level_and_id_opt->second);
    }

    // Calculate the unused id
    ::std::sort(id_used.begin(), id_used.end());
    for (auto&& [id1, id2] : id_used | rv::adjacent<2>)
    {
        for (file_id_t i = id1 + 1; i < id2; ++i)
        {
            m_id_recycled.push(i);
        }
    }

    // Entering working mode.
    m_working.fetch_add(1);
    co_return;
}

bool level::working() const noexcept
{
    return m_working.load() == 1;
}

} // namespace frenzykv
