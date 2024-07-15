// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "frenzykv/util/hash.h"
#include <functional>
#include <string_view>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "MurmurHash3.h"

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
murmur_bin_hash_x64_128_xor_shift_to_64::
operator()(::std::span<const ::std::byte> buffer) const noexcept
{
    static_assert(sizeof(uint64_t) == sizeof(::std::size_t));
    return operator()(buffer, 0);
}

::std::size_t 
murmur_bin_hash_x64_128_xor_shift_to_64::
operator()(::std::span<const ::std::byte> buffer, ::std::size_t hints) const noexcept
{
    static_assert(sizeof(uint64_t) == sizeof(::std::size_t));
    uint64_t h[2]{};
    MurmurHash3_x64_128(buffer.data(), static_cast<int>(buffer.size()), static_cast<uint32_t>(hints), h);
    return h[0] ^ (h[1] << 1);
}

::std::size_t 
murmur_bin_hash_x64_128_plus_to_64::
operator()(::std::span<const ::std::byte> buffer) const noexcept
{
    static_assert(sizeof(uint64_t) == sizeof(::std::size_t));
    return operator()(buffer, 0);
}

::std::size_t 
murmur_bin_hash_x64_128_plus_to_64::
operator()(::std::span<const ::std::byte> buffer, ::std::size_t hints) const noexcept
{
    static_assert(sizeof(uint64_t) == sizeof(::std::size_t));
    uint64_t h[2]{};
    MurmurHash3_x64_128(buffer.data(), static_cast<int>(buffer.size()), static_cast<uint32_t>(hints), h);
    return h[0] + h[1];
}

uint32_t murmur_bin_hash_x86_32::hash32(::std::span<const ::std::byte> buffer) const noexcept
{
    uint32_t result{};
    MurmurHash3_x86_32(buffer.data(), static_cast<int>(buffer.size()), 0, &result);
    return result;
}

uint32_t murmur_bin_hash_x86_32::hash32(::std::span<const ::std::byte> buffer, ::std::size_t hints) const noexcept
{
    uint32_t result{};
    MurmurHash3_x86_32(buffer.data(), static_cast<int>(buffer.size()), static_cast<uint32_t>(hints), &result);
    return result;
}

} // namespace frenzykv
