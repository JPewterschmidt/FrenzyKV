#include "log/logging_level.h"
#include "toolpex/in_there.h"
#include <stdexcept>

namespace frenzykv
{

::std::string_view to_string(logging_level l) noexcept
{
    switch (l)
    {
#define XXX(Level) case logging_level::Level: return #Level
        XXX(DEBUG);
        XXX(INFO);
        XXX(ERROR);
        XXX(ALERT);
        XXX(FATAL);
#undef XXX
    }
}

logging_level to_logging_level(::std::string_view lstr)
{
    using namespace toolpex;
    if (in_there(lstr, "debug", "LOG_DEBUG"))
        return logging_level::DEBUG;
    if (in_there(lstr, "info", "LOG_INFO"))
        return logging_level::INFO;
    if (in_there(lstr, "error", "LOG_ERROR"))
        return logging_level::ERROR;
    if (in_there(lstr, "alert", "LOG_ALERT"))
        return logging_level::ALERT;
    if (in_there(lstr, "fatal", "LOG_FATAL"))
        return logging_level::FATAL;
    throw ::std::out_of_range{"convert string to logging level error"};
}

} // namespace frenzykv
