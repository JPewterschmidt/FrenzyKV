#include <ranges>
#include <format>
#include <cassert>

#include "frenzykv/db/version_descriptor.h"
#include "frenzykv/util/uuid.h"

namespace rv = ::std::ranges::views;
using namespace ::std::string_view_literals;

namespace frenzykv
{

koios::task<bool> 
write_version_descriptor(const version_rep& version, const level& l, seq_writable* file)
{
    ::std::vector<::std::string> name_vec;
    for (const auto& guard : version.files())
    {
        name_vec.emplace_back(co_await l.file_name(guard));
    }
    co_return co_await write_version_descriptor(::std::move(name_vec), file);
}

koios::task<bool> 
write_version_descriptor(
    ::std::vector<::std::string> filenames, 
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
    co_await file->flush();
    co_return true;
}

koios::task<::std::vector<::std::string>> 
read_version_descriptor(seq_readable* file)
{
    ::std::vector<::std::string> result;
    size_t readed{1};
    while (readed)
    {
        // See also test/version.cc ::name_length
        ::std::array<::std::byte, 53> buffer{}; 
        readed = co_await file->read(buffer);
        if (readed) result.emplace_back(reinterpret_cast<char*>(buffer.data()), 52);
    }

    co_return result;
}

koios::task<> set_current_version_file(const kvdb_deps& deps, const ::std::string& filename)
{
    auto file = deps.env()->get_truncate_seq_writable(version_path()/"current_version_descriptor");
    co_await file->append(filename);
    co_await file->sync();
    
    co_return;
}

static constexpr auto vd_name_pattern = "frzkv#{}#.frzkvver"sv;

::std::string get_version_descriptor_name()
{
    return ::std::format(vd_name_pattern, uuid{}.to_string());
}

bool is_version_descriptor_name(::std::string_view name)
{
    return name.ends_with("#.frzkvver");
}

} // namespace frenzykv
