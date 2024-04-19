#ifndef FRENZYKV_OPTIONS_H
#define FRENZYKV_OPTIONS_H

#include <cstddef>
#include <filesystem>
#include <memory>
#include <limits>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "log/logging_level.h"
#include "frenzykv/statistics.h"

namespace frenzykv
{

class env;

struct options
{
    // XXX remember to update those serializer code below after you add something above.
    size_t disk_block_bytes = 4096;
    size_t memory_page_bytes = 4096;
    size_t max_block_segments_number = 1000;
    bool need_buffered_write = true;
    bool sync_write = false;
    bool buffered_read = true;
    bool need_compress = true;
    ::std::filesystem::path root_path = "/tmp/frenzykv/";
    ::std::filesystem::path log_path = "frenzy-prewrite-log";
    logging_level log_level = logging_level::DEBUG;
    ::std::string compressor_name = "zstd";
};

options get_global_options() noexcept;
void set_global_options(const nlohmann::json& j);
void set_global_options(const ::std::filesystem::path& filepath);

struct write_options
{
    bool sync_write = false;
};

}

NLOHMANN_JSON_NAMESPACE_BEGIN
template<>
struct adl_serializer<frenzykv::options> 
{
    static void to_json(json& j, const frenzykv::options& opt) 
    {
        j = json{ 
            { "disk_block_bytes",  opt.disk_block_bytes }, 
            { "memory_page_bytes", opt.memory_page_bytes },
            { "max_block_segments_number", opt.max_block_segments_number },
            { "need_compress", opt.need_compress },
            { "need_buffered_write", opt.need_buffered_write }, 
            { "sync_write", opt.sync_write }, 
            { "compressor_name", opt.compressor_name }, 
            { "buffered_read", opt.buffered_read }, 
            { "root_path", opt.root_path }, 
            { "log", {
                { "path", opt.log_path }, 
                { "level", opt.log_level }, 
            }}
        };
    }

    static void from_json(const json& j, frenzykv::options& opt) 
    {
        ::std::string temp;
        j.at("disk_block_bytes").get_to(opt.disk_block_bytes);
        j.at("memory_page_bytes").get_to(opt.memory_page_bytes);
        j.at("need_compress").get_to(opt.need_compress);
        j.at("need_buffered_write").get_to(opt.need_buffered_write);
        j.at("compressor_name").get_to(opt.compressor_name);
        j.at("max_block_segments_number").get_to(opt.max_block_segments_number);
        if (opt.max_block_segments_number > ::std::numeric_limits<uint16_t>::max())
        {
            spdlog::debug("TODO: the max_block_segments_number exceeds the limits. Something bad happenning.");
        }
        j.at("sync_write").get_to(opt.sync_write);
        j.at("buffered_read").get_to(opt.buffered_read);
        j.at("root_path").get_to(opt.root_path);
        temp.clear();
        j.at("log").at("path").get_to(temp);
        // TODO, assign to opt
        temp.clear();
        j.at("log").at("level").get_to(temp);
        // TODO, assign to opt
    }
};
NLOHMANN_JSON_NAMESPACE_END

#endif
