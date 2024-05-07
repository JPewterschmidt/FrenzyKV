#include <ranges>
#include <format>
#include <cassert>
#include <string>
#include <iterator>

#include "frenzykv/db/version_descriptor.h"
#include "frenzykv/util/uuid.h"
#include "frenzykv/util/file_center.h"

namespace r = ::std::ranges;
namespace rv = r::views;
using namespace ::std::string_view_literals;

namespace frenzykv
{

koios::task<bool> 
write_version_descriptor(const ::std::vector<file_guard>& filenames, seq_writable* file)
{
    auto names_view = filenames | rv::transform([](auto&& guard) { return ::std::string(guard); });
    ::std::vector<::std::string> names(begin(names_view), end(names_view));
    co_return co_await write_version_descriptor(names, file);
}

koios::task<bool> 
append_version_descriptor(const ::std::vector<file_guard>& filenames, seq_writable* file)
{
    auto names_view = filenames | rv::transform([](auto&& guard) { return ::std::string(guard); });
    ::std::vector<::std::string> names(begin(names_view), end(names_view));
    co_return co_await append_version_descriptor(names, file);
}

koios::task<bool> 
write_version_descriptor(const version_rep& version, seq_writable* file)
{
    assert(file->file_size() == 0);
    return append_version_descriptor(version, file);
}

koios::task<bool> 
write_version_descriptor(const ::std::vector<::std::string>& filenames, seq_writable* file)
{
    assert(file->file_size() == 0);
    return append_version_descriptor(filenames, file);
}

koios::task<bool> 
append_version_descriptor(const version_rep& version, seq_writable* file)
{
    ::std::vector<::std::string> name_vec;
    for (const auto& guard : version.files())
    {
        name_vec.emplace_back(guard);
    }
    co_return co_await append_version_descriptor(::std::move(name_vec), file);
}

koios::task<bool> 
append_version_descriptor(
    const ::std::vector<::std::string>& filenames, 
    seq_writable* file)
{
    constexpr auto newline = "\n"sv;
    for (const auto& name : filenames)
    {
        size_t wrote = co_await file->append(name);
        if (wrote != name.size())
            co_return false;
        co_await file->append(newline);
    }
    co_await file->sync();
    co_return true;
}

koios::task<::std::vector<::std::string>> 
read_version_descriptor(seq_readable* file)
{
    ::std::vector<::std::string> result;
    size_t readed{1};
    while (readed)
    {
        ::std::array<::std::byte, sst_name_length()> buffer{}; 
        readed = co_await file->read(buffer);
        if (readed < sst_name_length()) continue;
        if (readed) result.emplace_back(reinterpret_cast<char*>(buffer.data()), sst_name_length());
    }

    co_return result;
}

static constexpr auto vd_name_pattern = "frzkv#{}#.frzkvver"sv;
static constexpr size_t vd_name_length = 52; // See test of version descriptor

koios::task<> set_current_version_file(const kvdb_deps& deps, ::std::string_view filename)
{
    auto file = deps.env()->get_truncate_seq_writable(version_path()/current_version_descriptor_name());
    co_await file->append(filename);
    co_await file->sync();
    
    co_return;
}

koios::task<::std::optional<::std::string>> current_descriptor_name(const kvdb_deps& deps)
{
    auto file = deps.env()->get_seq_readable(version_path()/current_version_descriptor_name());
    ::std::string name(vd_name_length, 0);
    auto sp = ::std::as_writable_bytes(::std::span{name.data(), name.size()}.subspan(0, 52));
    const size_t readed = co_await file->read(sp);
    assert(readed == vd_name_length || readed == 0);
    if (readed == 0)
    {
        co_return {};
    }
    name.resize(vd_name_length);

    co_return name;
}

koios::task<version_delta> get_current_version(const kvdb_deps& deps, file_center* fc)
{
    auto name_opt = co_await current_descriptor_name(deps);
    if (!name_opt) co_return {};
    const auto& name = *name_opt;

    auto version_file = deps.env()->get_seq_readable(version_path()/name);
    assert(version_file->file_size() != 0);
    const auto name_vec = co_await read_version_descriptor(version_file.get());

    version_delta result;
    for (const auto& name : name_vec)
    {
        assert(is_sst_name(name));
        result.add_new_file(co_await fc->get_file(name));
    }
    co_return result;
}

::std::string get_version_descriptor_name()
{
    return ::std::format(vd_name_pattern, uuid{}.to_string());
}

bool is_version_descriptor_name(::std::string_view name)
{
    return name.ends_with("#.frzkvver");
}

} // namespace frenzykv
