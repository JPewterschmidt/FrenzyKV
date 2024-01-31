#include "frenzykv/options.h"
#include "toolpex/exceptions.h"

namespace frenzykv
{
    static options g_opt;

    const options& get_global_options() noexcept
    {
        return g_opt;
    }

    void set_global_options(const nlohmann::json& j)
    {
        g_opt = j.get<options>();
    }
}
