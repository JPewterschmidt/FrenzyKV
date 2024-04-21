#ifndef FRENZYKV_COMPRESSOR_H
#define FRENZYKV_COMPRESSOR_H

#include <string>
#include <memory>
#include <string_view>
#include <system_error>

#include "frenzykv/types.h"
#include "frenzykv/options.h"

namespace frenzykv
{

class compressor_policy
{
public:
    virtual ~compressor_policy() noexcept {}
    virtual ::std::string_view name() const noexcept = 0;

    virtual ::std::error_code compress(const_bspan original, ::std::string& compressed_dst) const = 0;
    virtual ::std::error_code decompress(const_bspan compressed_src, ::std::string& decompressed_dst) const = 0;

    virtual ::std::error_code compress_append_to(const_bspan original, ::std::string& compressed_dst) const = 0;
    virtual ::std::error_code decompress_append_to(const_bspan compressed_src, ::std::string& decompressed_dst) const = 0;

    virtual ::std::error_code compress(const_bspan original, bspan& compressed_dst) const = 0;
    virtual ::std::error_code decompress(const_bspan compressed_src, bspan& decompressed_dst) const = 0;

    virtual size_t decompressed_minimum_size(const_bspan original) const = 0;
    virtual size_t compressed_minimum_size(const_bspan original) const = 0;
};

::std::shared_ptr<compressor_policy> 
get_compressor(const options& opt, ::std::string_view name = "");

} // namespace frenzykv 

#endif
