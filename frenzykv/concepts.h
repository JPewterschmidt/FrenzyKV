#ifndef FERNZYKV_CONCEPTS_H
#define FERNZYKV_CONCEPTS_H

#include "frenzykv/write_batch.h"
#include <utility>

namespace frenzykv
{

template<typename Writer>
concept is_record_writer = requires (Writer w)
{
    w.write(::std::declval<write_batch>());
};

} // namespace frenzykv

#endif
