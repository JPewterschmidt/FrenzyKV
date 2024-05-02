#ifndef FRENZYKV_VERSION_DESCRUPTOR_H
#define FRENZYKV_VERSION_DESCRUPTOR_H

#include <memory>
#include <string>
#include <vector>

#include "koios/task.h"

#include "frenzykv/db/version.h"
#include "frenzykv/persistent/level.h"

namespace frenzykv
{

koios::task<bool> write_version_descriptor(const version_rep& version, const level& l, seq_writable* file);
koios::task<bool> write_version_descriptor(::std::vector<::std::string> filenames, seq_writable* file);
koios::task<::std::vector<::std::string>> read_version_descriptor(seq_readable* file);

} // namespace frenzykv

#endif
