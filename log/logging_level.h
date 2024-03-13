#ifndef FRENZYKV_LOGGING_LEVEL_H
#define FRENZYKV_LOGGING_LEVEL_H

#include <cstddef>

namespace frenzykv
{

enum logging_level : ::std::size_t
{
    LOG_DEBUG = 0, 
    LOG_INFO = 1, 
    LOG_ERROR = 2, 
    LOG_ALERT = 3, 
    LOG_FATAL = 4,
};

} // namespace frenzykv

#endif
