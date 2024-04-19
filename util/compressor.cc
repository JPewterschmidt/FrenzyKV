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
        compressed_dst.resize(::ZSTD_compressBound(original.size()), 0);

        const size_t sz_compressed = ::ZSTD_compress(
            compressed_dst.data(), compressed_dst.size(), 
            original.data(), original.size(), 
            ::ZSTD_maxCLevel()
        );

        if (::ZSTD_isError(sz_compressed))
        {
            return { static_cast<int>(sz_compressed), zstd_category() };
        }
        compressed_dst.resize(sz_compressed);

        return {};
    }

    ::std::error_code 
    decompress(const_bspan compressed_src, 
               ::std::string& decompressed_dst) const override
    {
        decompressed_dst.clear();
        decompressed_dst.resize(::ZSTD_getFrameContentSize(compressed_src.data(), compressed_src.size()));

        const size_t sz_decompr = ::ZSTD_decompress(
            decompressed_dst.data(), decompressed_dst.size(), 
            compressed_src.data(), compressed_src.size()
        );

        if (::ZSTD_isError(sz_decompr))
        {
            return { static_cast<int>(sz_decompr), zstd_category() };
        }
        decompressed_dst.resize(sz_decompr);

        return {};
    }
};

} // namespace frenzykv
