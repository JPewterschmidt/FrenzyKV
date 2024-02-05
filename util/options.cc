#include "frenzykv/options.h"
#include "toolpex/exceptions.h"
#include <fstream>
#include "koios/exceptions.h"

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

    void set_global_options(const ::std::filesystem::path& filepath)
    {
        ::std::ifstream ifs{ filepath };
        nlohmann::json content;
        if (!(ifs >> content).good())
            throw koios::exception{"cant' read option file!"};
        set_global_options(content);
    }
}
