#ifndef FRENZYKV_LOGGER_H
#define FRENZYKV_LOGGER_H

#include <string>
#include <string_view>
#include <filesystem>
#include "frenzykv/options.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/env.h"
#include "koios/task.h"
#include "koios/this_task.h"

#include "log/logging_level.h"

namespace frenzykv
{

class logger
{
public:
    logger(const options& opt, ::std::string filename)
        : m_opt{ &opt }, 
          m_filename{ ::std::move(filename) }, 
          m_filepath{ m_opt->root_path / m_filename }, 
          m_log_file{ m_opt->environment->get_seq_writable(m_filepath) }, 
          m_level{ m_opt->log_level }
    {
    }

    ::std::string to_string() const;
    ::std::string_view filename() const noexcept { return m_filename; }
    ::std::filesystem::path filepath() const { return m_filepath; }
    koios::task<> write(const write_batch& b);

    logging_level level() const noexcept { return m_level; }

    koios::task<> may_flush(bool force = false) noexcept;
    
private:
    const options* m_opt;
    ::std::string m_filename;
    ::std::filesystem::path m_filepath;
    ::std::unique_ptr<seq_writable> m_log_file;
    logging_level m_level;
};

template<logging_level Level>
class logging_record_writer
{
public:
    logging_record_writer(logger& parent) noexcept
        : m_parent{ &parent }
    {
    }

    koios::task<> write(const write_batch& batch)
    {
        if (level() >= m_parent->level())
            co_await m_parent->write(batch);
    }

    static constexpr logging_level level() noexcept { return Level; }

private:
    logger* m_parent{};
};

} // namespace frenzykv

#endif
