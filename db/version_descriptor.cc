// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <ranges>
#include <format>
#include <string>
#include <iterator>
#include <utility>

#include "toolpex/encode.h"
#include "toolpex/uuid.h"
#include "toolpex/assert.h"

#include "frenzykv/db/version_descriptor.h"
#include "frenzykv/util/file_center.h"

namespace r = ::std::ranges;
namespace rv = r::views;
using namespace ::std::string_view_literals;

namespace frenzykv
{

koios::task<bool> 
write_version_descriptor(const ::std::vector<file_guard>& filenames, seq_writable* file)
{
    co_return co_await write_version_descriptor(
        filenames 
            | rv::transform([](auto&& guard) { return guard.name(); }) 
            | r::to<::std::vector<::std::string>>()
            , 
        file
    );
}

koios::task<bool> 
append_version_descriptor(const ::std::vector<file_guard>& filenames, seq_writable* file)
{
    co_return co_await append_version_descriptor(
        filenames 
            | rv::transform([](auto&& guard) { return ::std::string(guard); }) 
            | r::to<::std::vector<::std::string>>()
            , 
        file
    );
}

koios::task<bool> 
write_version_descriptor(const version_rep& version, seq_writable* file)
{
    toolpex_assert(file->file_size() == 0);
    return append_version_descriptor(version, file);
}

koios::task<bool> 
write_version_descriptor(::std::vector<::std::string> filenames, seq_writable* file)
{
    toolpex_assert(file->file_size() == 0);
    return append_version_descriptor(::std::move(filenames), file);
}

koios::task<bool> 
append_version_descriptor(const version_rep& version, seq_writable* file)
{
    co_return co_await append_version_descriptor(
        version.files() 
            | rv::transform([](auto&& f){ return f.name(); })
            | r::to<::std::vector<::std::string>>()
            , 
        file
    );
}

koios::task<bool> 
append_version_descriptor(
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
        // buffer length = sst_name_length() + length of '\n'
        ::std::array<::std::byte, sst_name_length() + 1> buffer{}; 
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
    toolpex_assert(readed == vd_name_length || readed == 0);
    if (readed == 0)
    {
        co_return {};
    }
    name.resize(vd_name_length);

    co_return name;
}

koios::task<version_delta> get_version(const kvdb_deps& deps, ::std::string_view desc_name, file_center* fc)
{
    auto version_file = deps.env()->get_seq_readable(version_path()/desc_name);
    const auto name_vec = co_await read_version_descriptor(version_file.get());

    version_delta result;
    for (const auto& desc_name : name_vec)
    {
        toolpex_assert(is_sst_name(desc_name));
        result.add_new_file(co_await fc->get_file(desc_name));
    }
    co_return result;
}

koios::task<version_delta> get_current_version(const kvdb_deps& deps, file_center* fc)
{
    auto name_opt = co_await current_descriptor_name(deps);
    if (!name_opt) co_return {};
    co_return co_await get_version(deps, *name_opt, fc);
}

::std::string get_version_descriptor_name()
{
    return ::std::format(vd_name_pattern, toolpex::uuid{}.to_string());
}

bool is_version_descriptor_name(::std::string_view name)
{
    return name.ends_with("#.frzkvver");
}

static constexpr ::std::string_view seq_name = "seqnum";

koios::task<bool> 
write_leatest_sequence_number(const kvdb_deps& deps, sequence_number_t seq)
{
    auto env = deps.env();
    auto file = env->get_truncate_seq_writable(version_path()/seq_name);
    ::std::array<::std::byte, sizeof(sequence_number_t)> buffer{};
    toolpex::encode_little_endian_to(seq, buffer);
    if (co_await file->append(buffer) != sizeof(sequence_number_t))
        co_return false;

    co_await file->flush();

    co_return true;
}

koios::task<sequence_number_t> 
get_leatest_sequence_number(const kvdb_deps& deps)
{
    auto env = deps.env();
    auto file = env->get_seq_readable(version_path()/seq_name);
    ::std::array<::std::byte, sizeof(sequence_number_t)> buffer{};
    if (size_t readed = co_await file->read(buffer); readed == sizeof(sequence_number_t))
        co_return toolpex::decode_little_endian_from<sequence_number_t>(buffer);

    co_return 0;
}

} // namespace frenzykv
