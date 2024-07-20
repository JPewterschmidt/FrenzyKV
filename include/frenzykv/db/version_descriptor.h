// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_VERSION_DESCRUPTOR_H
#define FRENZYKV_VERSION_DESCRUPTOR_H

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include "koios/task.h"

#include "frenzykv/db/version.h"
#include "frenzykv/util/file_guard.h"
#include "frenzykv/util/file_center.h"

namespace frenzykv
{

koios::task<bool> write_version_descriptor(const version_rep& version, seq_writable* file);
koios::task<bool> append_version_descriptor(const version_rep& version, seq_writable* file);
koios::task<bool> write_version_descriptor(::std::vector<::std::string> filenames, seq_writable* file);
koios::task<bool> append_version_descriptor(::std::vector<::std::string> filenames, seq_writable* file);
koios::task<bool> write_version_descriptor(const ::std::vector<file_guard>& filenames, seq_writable* file);
koios::task<bool> append_version_descriptor(::std::vector<file_guard> filenames, seq_writable* file);

koios::task<::std::vector<::std::string>> read_version_descriptor(seq_readable* file);

koios::task<::std::optional<::std::string>> current_descriptor_name(const kvdb_deps& deps);
koios::task<> set_current_version_file(const kvdb_deps& deps, ::std::string_view filename);
koios::task<version_delta> get_current_version(const kvdb_deps& deps, file_center* fc);
koios::task<version_delta> get_version(const kvdb_deps& deps, ::std::string_view desc_name, file_center* fc);

::std::string get_version_descriptor_name();
bool is_version_descriptor_name(::std::string_view name);

koios::task<bool> write_leatest_sequence_number(const kvdb_deps& deps, sequence_number_t seq);
koios::task<sequence_number_t> get_leatest_sequence_number(const kvdb_deps& deps);

} // namespace frenzykv

#endif
