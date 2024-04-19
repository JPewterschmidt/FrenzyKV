#include "frenzykv/util/compressor.h"
#include "zstd.h"

namespace frenzykv
{

class zstd_category_t : public ::std::error_category
{
public:
    virtual const char* name() const noexcept override { return "ZSTD category"; }

    virtual ::std::string message(int condition) const override
    {
        const char* ename = ZSTD_getErrorName(condition);
        return ename;
    }
};

static const auto& zstd_category() noexcept
{
    static const zstd_category_t result;
    return result;
}

class zstd_compressor final : public compressor_policy
{
public:
    ::std::string_view name() const noexcept override { return "zstd_compressor"; }

    ::std::error_code 
    compress(const_bspan original, 
             ::std::string& compressed_dst) const override
    {
        compressed_dst.clear();
        return compress_append_to(original, compressed_dst);
    }

    ::std::error_code 
    decompress(const_bspan compressed_src, 
               ::std::string& decompressed_dst) const override
    {
        decompressed_dst.clear();
        return decompress_append_to(compressed_src, decompressed_dst);
    }

    ::std::error_code 
    compress_append_to(
        const_bspan original, 
        ::std::string& compressed_dst) const override
    {
        const size_t old_size = compressed_dst.size();
        const size_t compression_need_sz = ::ZSTD_compressBound(original.size());
        compressed_dst.resize(old_size + compression_need_sz, 0);

        const size_t sz_compressed = ::ZSTD_compress(
            compressed_dst.data() + old_size, compression_need_sz, 
            original.data(), original.size(), 
            ::ZSTD_maxCLevel()
        );

        if (::ZSTD_isError(sz_compressed))
        {
            return { static_cast<int>(sz_compressed), zstd_category() };
        }
        compressed_dst.resize(old_size + sz_compressed);

        return {};
    }

    ::std::error_code 
    decompress_append_to(
        const_bspan compressed_src, 
        ::std::string& decompressed_dst) const override
    {
        const size_t old_size = decompressed_dst.size();
        const size_t decompression_need_sz = ::ZSTD_getFrameContentSize(
            compressed_src.data(), compressed_src.size()
        );
        decompressed_dst.resize(old_size + decompression_need_sz);

        const size_t sz_decompr = ::ZSTD_decompress(
            decompressed_dst.data() + old_size, decompression_need_sz,
            compressed_src.data(), compressed_src.size()
        );

        if (::ZSTD_isError(sz_decompr))
        {
            return { static_cast<int>(sz_decompr), zstd_category() };
        }
        decompressed_dst.resize(old_size + sz_decompr);

        return {};
    }
};

class empty_compressor final : public compressor_policy
{
public:
    ::std::string_view name() const noexcept override { return "empty_compressor"; }

    ::std::error_code 
    compress(const_bspan original, 
             ::std::string& compressed_dst) const override
    {
        compressed_dst.clear();
        return compress_append_to(original, compressed_dst);
    }

    ::std::error_code 
    decompress(const_bspan compressed_src, 
               ::std::string& decompressed_dst) const override
    {
        decompressed_dst.clear();
        return decompress_append_to(compressed_src, decompressed_dst);
    }

    ::std::error_code 
    compress_append_to(
        const_bspan original, 
        ::std::string& compressed_dst) const override
    {
        compressed_dst.append(as_string_view(original));
        return {};
    }

    ::std::error_code 
    decompress_append_to(
        const_bspan compressed_src, 
        ::std::string& decompressed_dst) const override
    {
        decompressed_dst.append(as_string_view(compressed_src));
        return {};
    }
};

::std::shared_ptr<compressor_policy> 
get_compressor(const options& opt, ::std::string_view name)
{
    static const ::std::unordered_map<
        ::std::string, 
        ::std::shared_ptr<compressor_policy>
    > compressors = 
    []{ 
        ::std::unordered_map<
            ::std::string, 
            ::std::shared_ptr<compressor_policy>
        > result 
        { 
            { "zstd", ::std::make_shared<zstd_compressor>() }, 
            { "empty", ::std::make_shared<empty_compressor>() },
        };

        return result;
    }();

    if (name.empty())
    {
        name = opt.compressor_name;
    }

    assert(compressors.contains(::std::string{name}));
    return compressors.at(::std::string{name});
}

} // namespace frenzykv