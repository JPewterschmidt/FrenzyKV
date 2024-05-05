#ifndef FRENZYKV_VERSION_DESCRUPTOR_H
#define FRENZYKV_VERSION_DESCRUPTOR_H

#include <memory>
#include <string>
#include <vector>

#include "koios/task.h"

#include "frenzykv/db/version.h"
#include "frenzykv/util/file_guard.h"

namespace frenzykv
{

koios::task<bool> write_version_descriptor(const version_rep& version, seq_writable* file);
koios::task<bool> append_version_descriptor(const version_rep& version, seq_writable* file);
koios::task<bool> write_version_descriptor(const ::std::vector<::std::string>& filenames, seq_writable* file);
koios::task<bool> append_version_descriptor(const ::std::vector<::std::string>& filenames, seq_writable* file);
koios::task<bool> write_version_descriptor(const ::std::vector<file_guard>& filenames, seq_writable* file);
koios::task<bool> append_version_descriptor(const ::std::vector<file_guard>& filenames, seq_writable* file);

koios::task<::std::vector<::std::string>> read_version_descriptor(seq_readable* file);

koios::task<> set_current_version_file(const kvdb_deps& deps, const ::std::string& filename);
koios::task<version_delta> get_current_version(const kvdb_deps& deps);

::std::string get_version_descriptor_name();
bool is_version_descriptor_name(::std::string_view name);

} // namespace frenzykv

#endif
