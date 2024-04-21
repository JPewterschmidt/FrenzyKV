#include "frenzykv/util/compressor.h"
#include "frenzykv/error_category.h"
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
private:
    int m_level{};
    int compress_level() const noexcept { return m_level; }

public:
    zstd_compressor(int compress_level = 15)
        : m_level{ compress_level }
    {
        if (m_level < ::ZSTD_minCLevel() || m_level > ::ZSTD_maxCLevel())
            throw ::std::out_of_range("zstd_compressor: level out of range [1, 22]");
    }

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
    compress(const_bspan original, bspan& compressed_dst) const override
    {
        const size_t space_need = compressed_minimum_size(original);
        if (compressed_dst.size() < space_need)
            return make_frzkv_out_of_range();

        const size_t sz_compressed = ::ZSTD_compress(
            compressed_dst.data(), compressed_dst.size(), 
            original.data(), original.size(), 
            compress_level()
        );

        if (::ZSTD_isError(sz_compressed))
        {
            return { static_cast<int>(sz_compressed), zstd_category() };
        }

        compressed_dst = compressed_dst.subspan(0, sz_compressed);
        return {};
    }

    ::std::error_code 
    decompress(const_bspan compressed_src, bspan& decompressed_dst) const override
    {
        const size_t space_need = decompressed_minimum_size(compressed_src);
        if (decompressed_dst.size() < space_need)
            return make_frzkv_out_of_range();

        const size_t sz_decompr = ::ZSTD_decompress(
            decompressed_dst.data(), decompressed_dst.size(),
            compressed_src.data(), compressed_src.size()
        );

        if (::ZSTD_isError(sz_decompr))
        {
            return { static_cast<int>(sz_decompr), zstd_category() };
        }
        
        decompressed_dst = decompressed_dst.subspan(0, sz_decompr);
        return {};
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
            compress_level()
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
        const size_t decompression_need_sz = decompressed_minimum_size(compressed_src);
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

    size_t decompressed_minimum_size(const_bspan original) const override
    {
        return ::ZSTD_getFrameContentSize(original.data(), original.size());
    }

    size_t compressed_minimum_size(const_bspan original) const override
    {
        return ::ZSTD_compressBound(original.size());
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

    size_t decompressed_minimum_size(const_bspan original) const noexcept override
    {
        return original.size();
    }

    size_t compressed_minimum_size(const_bspan original) const noexcept override
    {
        return original.size();
    }

    ::std::error_code 
    compress(const_bspan original, bspan& compressed_dst) const override
    {
        if (compressed_dst.size() < original.size()) [[unlikely]]
            return make_frzkv_out_of_range();
        ::std::memcpy(compressed_dst.data(), original.data(), original.size());
        compressed_dst = compressed_dst.subspan(0, original.size());
        return {};
    }

    ::std::error_code 
    decompress(const_bspan compressed_src, bspan& decompressed_dst) const override
    {
        if (decompressed_dst.size() < compressed_src.size()) [[unlikely]]
            return make_frzkv_out_of_range();
        ::std::memcpy(decompressed_dst.data(), compressed_src.data(), compressed_src.size());
        decompressed_dst = decompressed_dst.subspan(0, compressed_src.size());
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
    [](const auto& opt){ 
        ::std::unordered_map<
            ::std::string, 
            ::std::shared_ptr<compressor_policy>
        > result 
        { 
            { "zstd", ::std::make_shared<zstd_compressor>(opt.compress_level) }, 
            { "empty", ::std::make_shared<empty_compressor>() },
        };

        return result;
    }(opt);

    if (name.empty())
    {
        name = opt.compressor_name;
    }

    assert(compressors.contains(::std::string{name}));
    return compressors.at(::std::string{name});
}

} // namespace frenzykv
