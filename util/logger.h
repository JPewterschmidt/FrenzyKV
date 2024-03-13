#ifndef FRENZYKV_LOGGER_H
#define FRENZYKV_LOGGER_H

#include <string>
#include <string_view>
#include <filesystem>
#include "frenzykv/options.h"
#include "frenzykv/write_batch.h"
#include "koios/task.h"

namespace frenzykv
{

// Apprently, the class is a record_writer
class logger
{
public:
    logger(const options& opt, ::std::string filename)
        : m_opt{ &opt }, m_filename{ ::std::move(filename) }
    {
    }

    ::std::string to_string() const;
    ::std::string_view filename() const noexcept { return m_filename; }
    ::std::filesystem::path filepath() const { return m_filepath; }
    koios::task<> write(const write_batch& b);
    
private:
    const options* m_opt;
    ::std::string m_filename;
    ::std::filesystem::path m_filepath;
    ::std::unique_ptr<seq_writable> m_log_file;
};

} // namespace frenzykv

#endif
