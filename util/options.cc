#include "frenzykv/options.h"
#include "toolpex/exceptions.h"
#include <fstream>
#include <utility>
#include <memory>
#include "koios/exceptions.h"

using namespace ::std::chrono_literals;

namespace frenzykv
{
    static ::std::unique_ptr<nlohmann::json> g_data;

    options::options()
        : disk_block_bytes{ 4096 }, 
          memory_page_bytes{ 4096 }, 
          max_block_segments_number{ 1000 }, 
          block_size{ 4096 }, 
          max_level{ 7 }, 
          level_file_number{ 4, 8, 16, 16, 16, 16 }, 

          // SSTable bound
          level_file_size{ 
              16 * 1024 * 1024,  
              32 * 1024 * 1024,  
              64 * 1024 * 1024,  
              128 * 1024 * 1024,  
              256 * 1024 * 1024,  
              512 * 1024 * 1024,  
          }, 
          compress_level{ 15 },
          need_buffered_write{ true },
          sync_write{ false },
          buffered_read{ true },
          need_compress{ true },
          gc_period_sec{ 10s },
          root_path{ "/tmp/frenzykv" },
          create_root_path_if_not_exists{ true },
          log_path{ "frenzy-prewrite-log" },
          log_level{ logging_level::INFO },
          compressor_name{ "zstd" }
    {
    }

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
        if (static_cast<size_t>(l) >= level_file_number.size()) return 0;
        return level_file_number[l];
    }

    size_t options::allowed_level_file_size(level_t l) const noexcept
    {
        if (static_cast<size_t>(l) >= level_file_size.size()) return 0;
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
