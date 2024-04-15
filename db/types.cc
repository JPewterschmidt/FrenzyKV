#include "frenzykv/types.h"

namespace frenzykv
{

::std::string_view as_string_view(const_bspan s)
{
    return { reinterpret_cast<const char*>(s.data()), s.size() };
}

} // namespace frenzykv
