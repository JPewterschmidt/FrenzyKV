#include "util/logger.h"
#include "toolpex/functional.h"
#include <format>

namespace frenzykv
{

::std::string logger::to_string() const
{
    return ::std::format("logger: name = {}, path = {}", 
        filename(), filepath().string());
}

koios::task<> logger::write(const write_batch& b)
{
    co_await m_log_file->append(b.to_string_log());
}

} // namespace frenzykv
