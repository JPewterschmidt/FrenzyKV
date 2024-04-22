#ifndef FRENZYKV_IO_FILE_H
#define FRENZYKV_IO_FILE_H

#include <cstddef>
#include <cstdint>

#include "koios/task.h"

namespace frenzykv
{

class file
{
public:
    virtual ~file() noexcept { }
    virtual uintmax_t file_size() const = 0;
    virtual bool is_buffering() const = 0;
    virtual koios::task<> close() = 0;
};

} // namespace frenzykv

#endif
