#ifndef FRENZYKV_READABLE_H
#define FRENZYKV_READABLE_H

#include "toolpex/move_only.h"
#include "koios/task.h"

#include <span>
#include <cstddef>

namespace frenzykv
{

class readable
{
public:
    virtual koios::task<::std::error_code> 
    read(::std::span<::std::byte>, size_t offset) const = 0;
};

}

#endif
