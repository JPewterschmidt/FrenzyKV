#include "frenzykv/db.h"

namespace frenzykv
{

::std::unique_ptr<db_interface> open(toolpex::ip_address::ptr ip, in_port_t port)
{
    return nullptr;
}

::std::unique_ptr<db_interface> open(::std::filesystem::path path, options opt)
{
    return nullptr;
}

} // namespace frenzykv
