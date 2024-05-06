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

::std::string prewrite_log_name();

class logger
{
public:
    logger(const kvdb_deps& deps)
        : m_deps{ &deps }, 
          m_log_file{ m_deps->env()->get_seq_writable(prewrite_log_path()/prewrite_log_name()) }
    {
    }

    koios::task<> insert(const write_batch& b);
    koios::task<> may_flush(bool force = false) noexcept;
    koios::task<bool> empty() const noexcept;
    koios::task<> truncate_file() noexcept;
    koios::eager_task<> delete_file();
    
private:
    const kvdb_deps* m_deps{};
    ::std::unique_ptr<seq_writable> m_log_file;
};

koios::task<write_batch> recover(env* e) noexcept;

} // namespace frenzykv

#endif
