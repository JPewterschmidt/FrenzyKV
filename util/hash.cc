#include "util/hash.h"
#include <functional>
#include <string_view>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace frenzykv
{

::std::size_t 
std_bin_hash::
operator()(::std::span<const ::std::byte> buffer) const noexcept
{
    ::std::string_view contents{ 
        reinterpret_cast<const char*>(buffer.data()), 
        buffer.size() 
    };
    return ::std::hash<::std::string_view>{}(contents);
}

::std::size_t 
std_bin_hash::
operator()(::std::span<const ::std::byte> buffer, ::std::size_t hints) const noexcept
{
    return this->hash(buffer);
}

::std::size_t 
murmur_bin_hash::
operator()(::std::span<const ::std::byte> buffer) const noexcept
{
    return hash(buffer, 0);
}

::std::size_t murmur_hash3_32bits(
    const void* key, 
    ::std::size_t len, 
    ::std::size_t seed = 0) noexcept
{
    constexpr ::std::size_t m = 0x5bd1e995;
    constexpr int r = 24;

    const uint8_t* data = reinterpret_cast<const uint8_t*>(key);
    const uint8_t* end = data + len;

    ::std::size_t h = seed ^ len;

    while (data + 4 <= end) {
        ::std::size_t k{};
        std::memcpy(&k, data, ::std::min(sizeof(::std::size_t), len));
        data += 4;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;
    }

    if (data < end) {
        ::std::size_t k = 0;
        switch (end - data) {
            case 3: k ^= data[2] << 16; [[fallthrough]];
            case 2: k ^= data[1] << 8;  [[fallthrough]];
            case 1: k ^= data[0];
                    k *= m;
        };

        k ^= k >> r;
        k *= m;
        h ^= k;
    }

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

::std::size_t 
murmur_bin_hash::
operator()(::std::span<const ::std::byte> buffer, ::std::size_t hints) const noexcept
{
    return murmur_hash3_32bits(buffer.data(), buffer.size(), hints);
}

} // namespace frenzykv
