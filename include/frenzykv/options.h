#ifndef FRENZYKV_OPTIONS_H
#define FRENZYKV_OPTIONS_H

#include <cstddef>
#include <filesystem>
#include <memory>
#include <limits>
#include <chrono>

#include "magic_enum.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "log/logging_level.h"
#include "frenzykv/statistics.h"
#include "frenzykv/types.h"

namespace frenzykv
{

class env;

struct options
{
    options();

    // XXX remember to update those serializer code below after you add something above.
    size_t  disk_block_bytes;
    size_t  memory_page_bytes;
    size_t  max_block_segments_number;
    size_t  block_size;
    size_t  max_level;

    ::std::vector<size_t> level_file_number;
    ::std::vector<size_t> level_file_size; // SSTable bound

    int     compress_level;
    bool    need_buffered_write;
    bool    sync_write;
    bool    buffered_read;
    bool    need_compress;

    ::std::chrono::seconds      gc_period_sec;
    ::std::filesystem::path     root_path;
    bool                        create_root_path_if_not_exists;
    ::std::filesystem::path     log_path;
    logging_level               log_level;
    ::std::string               compressor_name;

    size_t allowed_level_file_number(level_t l) const noexcept;
    size_t allowed_level_file_size(level_t l) const noexcept;
    bool is_appropriate_level_file_number(level_t l, size_t num) const noexcept;
    bool is_appropriate_level_file_size(level_t l, size_t num) const noexcept;
};

options get_global_options() noexcept;
void set_global_options(nlohmann::json j);
void set_global_options(const ::std::filesystem::path& filepath);

}

NLOHMANN_JSON_NAMESPACE_BEGIN
template<>
struct adl_serializer<frenzykv::options> 
{
    static void to_json(json& j, const frenzykv::options& opt) 
    {
        j = json{ 
            { "max_level", opt.max_level },
            { "level_file_number", opt.level_file_number }, 
            { "level_file_size", opt.level_file_size }, 
            { "disk_block_bytes",  opt.disk_block_bytes }, 
            { "memory_page_bytes", opt.memory_page_bytes },
            { "max_block_segments_number", opt.max_block_segments_number },
            { "block_size", opt.block_size },
            { "need_compress", opt.need_compress },
            { "need_buffered_write", opt.need_buffered_write }, 
            { "gc_period_sec", opt.gc_period_sec.count() }, 
            { "compress_level", opt.compress_level }, 
            { "sync_write", opt.sync_write }, 
            { "compressor_name", opt.compressor_name }, 
            { "buffered_read", opt.buffered_read }, 
            { "root_path", { 
                { "path", opt.root_path }, 
                { "create_root_path_if_not_exists", opt.create_root_path_if_not_exists },
            }}, 
            { "log", {
                { "path", opt.log_path }, 
                { "level", magic_enum::enum_name<frenzykv::logging_level>(opt.log_level) }, 
            }}, 
        };
    }

    static void from_json(const json& j, frenzykv::options& opt) 
    {
        ::std::string temp;
        j.at("disk_block_bytes").get_to(opt.disk_block_bytes);
        j.at("memory_page_bytes").get_to(opt.memory_page_bytes);
        j.at("need_compress").get_to(opt.need_compress);
        j.at("need_buffered_write").get_to(opt.need_buffered_write);
        j.at("max_level").get_to(opt.max_level);
        j.at("block_size").get_to(opt.block_size);
        j.at("level_file_number").get_to(opt.level_file_number);
        j.at("level_file_size").get_to(opt.level_file_size);

        int sec{10};
        j.at("gc_period_sec").get_to(sec);
        opt.gc_period_sec = ::std::chrono::seconds{sec};

        j.at("compressor_name").get_to(opt.compressor_name);
        j.at("compress_level").get_to(opt.compress_level);
        j.at("max_block_segments_number").get_to(opt.max_block_segments_number);
        if (opt.max_block_segments_number > ::std::numeric_limits<uint16_t>::max())
        {
            spdlog::debug("TODO: the max_block_segments_number exceeds the limits. Something bad happenning.");
        }
        j.at("sync_write").get_to(opt.sync_write);
        j.at("buffered_read").get_to(opt.buffered_read);
        j.at("root_path").at("path").get_to(opt.root_path);
        j.at("root_path").at("create_root_path_if_not_exists").get_to(opt.create_root_path_if_not_exists);
        temp.clear();
        j.at("log").at("path").get_to(temp);
        temp.clear();
        
        ::std::string level_str;
        j.at("log").at("level").get_to(level_str);
        auto logl_opt = magic_enum::enum_cast<frenzykv::logging_level>(level_str);
        if (logl_opt)
        {
            opt.log_level = logl_opt.value();
        }
        else
        {
            spdlog::error("could not retrive logging level from your conf file.");
        }

        if (opt.level_file_number.size() < opt.max_level - 1)
            spdlog::error("wrong file number count");
        
        if (opt.level_file_size.size() < opt.max_level - 1)
            spdlog::error("wrong file size number");
    }
};
NLOHMANN_JSON_NAMESPACE_END

#endif
