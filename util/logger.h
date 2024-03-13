#ifndef FRENZYKV_LOGGER_H
#define FRENZYKV_LOGGER_H

#include <string>
#include <string_view>
#include <filesystem>
#include "frenzykv/options.h"
#include "frenzykv/write_batch.h"
#include "koios/task.h"

#include "log/logging_level.h"

namespace frenzykv
{

class logger
{
public:
    logger(const options& opt, ::std::string filename)
        : m_opt{ &opt }, m_filename{ ::std::move(filename) }
    {
    }

    ::std::string to_string() const
    {
        return ::std::format("logger: name = {}, path = {}", 
            filename(), filepath().string());
    }

    ::std::string_view filename() const noexcept { return m_filename; }
    ::std::filesystem::path filepath() const { return m_filepath; }
    koios::task<> write(const write_batch& b)
    {
        co_await m_log_file->append(b.to_string_log());
    }

    static constexpr logging_level level() noexcept { return Level; }
    
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
    template<typename PLevel>
    logging_record_writer(logger<PLevel> 

private:
};


} // namespace frenzykv

#endif
