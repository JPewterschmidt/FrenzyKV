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
namespace r = ::std::ranges;
namespace rv = r::views;
using namespace ::std::string_literals;
using namespace ::std::string_view_literals;

::std::string name_a_sst(level_t l, const file_id_t& id)
{
    return ::std::format("frzkv#{}#{}#.frzkvsst", l, id.to_string());
}

static bool is_name_allcated_here(const ::std::string& name)
{
    return name.starts_with("frzkv#") && name.ends_with("#.frzkvsst");   
}

::std::optional<::std::pair<level_t, file_id_t>>
retrive_level_and_id_from_sst_name(const ::std::string& name)
{
    ::std::optional<::std::pair<level_t, file_id_t>> result;
    if (!is_name_allcated_here(name))
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

koios::task<::std::string> level::file_name(const file_guard& f) const
{
    auto lk = co_await m_mutex.acquire_shared();
    co_return m_id_name.at(f.file_id());
}

level::level(const kvdb_deps& deps) 
   : m_deps{ &deps }, 
     m_levels_file_rep(m_deps->opt()->max_level)
{
}

koios::task<size_t> level::allowed_file_number(level_t l) const noexcept
{
    auto opt = m_deps->opt();
    const auto& number_vec = opt->level_file_number;
    if (l >= number_vec.size()) co_return 0;
    co_return number_vec[l];
}

koios::task<size_t> level::allowed_file_size(level_t l) const noexcept
{
    auto opt = m_deps->opt();
    const auto& number_vec = opt->level_file_size;
    if (l >= number_vec.size()) co_return 0;
    co_return number_vec[l];
}

koios::task<::std::pair<file_guard, ::std::unique_ptr<seq_writable>>> 
level::create_file(level_t level)
{
    auto shr = co_await m_mutex.acquire_shared();
    assert(level < m_levels_file_rep.size());
    assert(working());
    file_id_t id{};
    auto name = name_a_sst(level, id);
    shr.unlock();

    auto file = m_deps->env()->get_seq_writable(sstables_path()/name);

    auto lk = co_await m_mutex.acquire();
    m_id_name[id] = name;
    auto& rep = m_levels_file_rep[level].emplace_back(::std::make_unique<file_rep>(level, id, name));

    co_return { *rep, ::std::move(file) };
}

koios::task<> level::delete_file(const file_rep& rep)
{
    assert(working());
    assert(rep.approx_ref_count() == 0);

    auto shr = co_await m_mutex.acquire_shared();
    const auto name = m_id_name.at(rep);
    shr.unlock();
    co_await m_deps->env()->delete_file(sstables_path()/name);

    auto lk = co_await m_mutex.acquire();
    m_id_name.erase(rep);
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

    // Go through all the files.
    for (const auto& dir_entry : fs::directory_iterator{ sstables_path() })
    {
        auto name = dir_entry.path().filename().string();
        auto level_and_id_opt = retrive_level_and_id_from_sst_name(name);
        if (!level_and_id_opt) 
            continue;
        m_id_name[level_and_id_opt->second] = name;

        m_levels_file_rep[level_and_id_opt->first]
            .emplace_back(::std::make_unique<file_rep>(level_and_id_opt->first, level_and_id_opt->second, name));
    }

    // Entering working mode.
    m_working.fetch_add(1);
    co_return;
}

bool level::working() const noexcept
{
    return m_working.load() == 1;
}

koios::task<size_t> level::actual_file_number(level_t l) const noexcept
{
    auto lk = co_await m_mutex.acquire_shared();
    assert(l < m_levels_file_rep.size());
    co_return m_levels_file_rep[l].size();
}

koios::task<bool> level::need_to_comapct(level_t l) const noexcept
{
    co_return (co_await actual_file_number(l) >= co_await allowed_file_number(l));
}

koios::task<::std::vector<file_guard>> level::level_file_guards(level_t l) noexcept
{
    auto lk = co_await m_mutex.acquire_shared();
    assert(l < m_levels_file_rep.size());
    auto v = m_levels_file_rep[l] 
        | rv::transform([](auto&& rep) { return file_guard(*rep); });
    co_return { begin(v), end(v) };
}

koios::task<size_t> level::level_number() const noexcept
{
    auto lk = co_await m_mutex.acquire_shared();
    co_return m_levels_file_rep.size();
}

koios::task<file_guard> level::oldest_file(level_t l)
{
    co_return co_await oldest_file(co_await level_file_guards(l));
}

koios::task<file_guard> level::oldest_file(const ::std::vector<file_guard>& files)
{
    ::std::pair<fs::file_time_type, file_guard> oldest;
    auto lk = co_await m_mutex.acquire_shared();
    auto tfps = files 
        | rv::transform([this](auto&& guard){ 
              return ::std::pair{ m_id_name.at(file_id_t(guard)), guard }; 
          })
        | rv::transform([](auto&& name_guard) { 
              return ::std::pair{ 
                  fs::last_write_time(name_guard.first), name_guard.second 
              }; 
          });
    oldest = *begin(tfps);

    // TODO: optimize: do copy elision 
    for (auto p : tfps)
    {
        if (p.first < oldest.first)
            oldest = p;
    }

    co_return ::std::move(oldest.second);
}

koios::task<::std::unique_ptr<seq_writable>>
level::
open_write(const file_guard& guard)
{
    auto lk = co_await m_mutex.acquire_shared();
    if (!co_await level_contains(guard)) [[unlikely]]
    {
        co_return nullptr;
    }
    auto result = m_deps->env()->get_seq_writable(
        sstables_path()/m_id_name[guard]
    );
    assert(result->file_size() == 0);
    co_return result;
}

koios::task<::std::unique_ptr<random_readable>>
level::
open_read(const file_guard& guard)
{
    auto lk = co_await m_mutex.acquire_shared();
    if (!co_await level_contains(guard)) [[unlikely]]
    {
        co_return nullptr;
    }
    auto result = m_deps->env()->get_random_readable(
        sstables_path()/m_id_name[guard]
    );
    assert(result->file_size() > 0);
    co_return result;
}

koios::task<bool> level::level_contains(const file_rep& rep) const
{
    auto lk = co_await m_mutex.acquire_shared();
    const auto& vec = m_levels_file_rep[rep];
    for (const auto& rep_ptr : vec)
    {
        if (*rep_ptr == rep) co_return true;
    }
    co_return false;
}

koios::task<> level::GC()
{
    auto lk = co_await m_mutex.acquire();
    for (auto& level_files : m_levels_file_rep)
    {
        auto remove_ret = r::remove_if(level_files, [](const auto& filep) {
            assert(filep != nullptr);
            return filep->approx_ref_count() == 0;
        });
        for (auto& filep : remove_ret)
        {
            co_await delete_file(*filep);
        }
        level_files.erase(remove_ret.begin(), remove_ret.end());
    }
    co_return;
}

} // namespace frenzykv
