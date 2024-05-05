#include "frenzykv/options.h"
#include "toolpex/exceptions.h"
#include <fstream>
#include <utility>
#include <memory>
#include "koios/exceptions.h"

namespace frenzykv
{
    static ::std::unique_ptr<nlohmann::json> g_data;

    options get_global_options() noexcept
    {
        if (g_data) 
        {
            return g_data->get<options>();
        }
        return {};
    }

    void set_global_options(nlohmann::json j)
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

    size_t options::allowed_level_file_number(level_t l) const noexcept
    {
        if (l >= level_file_number.size()) return 0;
        return level_file_number[l];
    }

    size_t options::allowed_level_file_size(level_t l) const noexcept
    {
        if (l >= level_file_size.size()) return 0;
        return level_file_size[l];
    }

    bool options::is_appropriate_level_file_number(level_t l, size_t num) const noexcept
    {
        const size_t val = allowed_level_file_number(l);
        return val >= num || val == 0;
    }

    bool options::is_appropriate_level_file_size(level_t l, size_t num) const noexcept
    {
        const size_t val = allowed_level_file_size(l);
        return val >= num || val == 0;
    }
}
