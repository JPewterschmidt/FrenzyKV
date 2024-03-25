#include "frenzykv/options.h"
#include "toolpex/exceptions.h"
#include <fstream>
#include <utility>
#include <memory>
#include "koios/exceptions.h"

namespace frenzykv
{
    static ::std::unique_ptr<nlohmann::json> g_data;

    options get_global_options(env* e) noexcept
    {
        if (g_data) 
        {
            auto result = g_data->get<options>();
            result.environment = e;
            return result;
        }
        return { .environment = e };
    }

    void set_global_options(nlohmann::json& j)
    {
        g_data = ::std::make_unique<nlohmann::json>(::std::move(j));
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
