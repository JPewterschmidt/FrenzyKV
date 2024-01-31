#ifndef FRENZYKV_OPTIONS_H
#define FRENZYKV_OPTIONS_H

#include <cstddef>
#include <filesystem>

#include "nlohmann/json.hpp"

namespace frenzykv
{
struct options
{
    size_t disk_block_bytes = 4096;
    size_t memory_page_bytes = 4096;
    bool need_buffered_write = true;
    bool sync_write = false;
    bool buffered_read = true;
};

const options& get_global_options() noexcept;
void set_global_options(const nlohmann::json& j);
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
            { "need_buffered_write", opt.need_buffered_write }, 
            { "sync_write", opt.sync_write }, 
            { "buffered_read", opt.buffered_read }, 
        };
    }

    static void from_json(const json& j, frenzykv::options& opt) 
    {
        j.at("disk_block_bytes").get_to(opt.disk_block_bytes);
        j.at("memory_page_bytes").get_to(opt.memory_page_bytes);
        j.at("need_buffered_write").get_to(opt.need_buffered_write);
        j.at("sync_write").get_to(opt.sync_write);
        j.at("buffered_read").get_to(opt.buffered_read);
    }
};
NLOHMANN_JSON_NAMESPACE_END

#endif
