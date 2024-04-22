#ifndef FRENZYKV_LOGGER_H
#define FRENZYKV_LOGGER_H

#include <string>
#include <string_view>
#include <filesystem>
#include "frenzykv/kvdb_deps.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/env.h"
#include "frenzykv/log/logging_level.h"
#include "koios/task.h"
#include "koios/this_task.h"

namespace frenzykv
{

class logger
{
public:
    logger(const kvdb_deps& deps, ::std::unique_ptr<seq_writable> file)
        : m_deps{ &deps }, 
          m_log_file{ ::std::move(file) }
    {
    }

    koios::task<> write(const write_batch& b);

    koios::task<> may_flush(bool force = false) noexcept;
    
private:
    const kvdb_deps* m_deps{};
    ::std::unique_ptr<seq_writable> m_log_file;
};

} // namespace frenzykv

#endif
