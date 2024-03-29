#ifndef FRENZYKV_OPTIONS_H
#define FRENZYKV_OPTIONS_H

#include <cstddef>
#include <filesystem>
#include <memory>

#include "nlohmann/json.hpp"

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
    size_t memtable_size_bound = 32768;
    bool need_buffered_write = true;
    bool sync_write = false;
    bool buffered_read = true;
    ::std::filesystem::path root_path = "./";
    ::std::filesystem::path log_path = "frenzy-prewrite-log";
    logging_level log_level = logging_level::DEBUG;

    // Need not serializer
    env* environment{};
    statistics* stat{};   
};

options get_global_options(env* e = nullptr) noexcept;
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
            { "need_buffered_write", opt.need_buffered_write }, 
            { "sync_write", opt.sync_write }, 
            { "buffered_read", opt.buffered_read }, 
            { "root_path", opt.root_path }, 
            { "memtable_size_bound", opt.memtable_size_bound }, 
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
        j.at("need_buffered_write").get_to(opt.need_buffered_write);
        j.at("sync_write").get_to(opt.sync_write);
        j.at("buffered_read").get_to(opt.buffered_read);
        j.at("memtable_size_bound").get_to(opt.memtable_size_bound);
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
