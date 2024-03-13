#ifndef FRENZYKV_NOP_RECORD_WRITER_H
#define FRENZYKV_NOP_RECORD_WRITER_H

#include "frenzykv/write_batch.h"
#include <coroutine>

namespace frenzykv
{

class nop_record_writer
{
public:
    constexpr nop_record_writer() noexcept = default;
    constexpr ::std::suspend_never write([[maybe_unused]] const write_batch&) const noexcept { return {}; }
};

} // namespace frenzykv

#endif
