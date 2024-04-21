#ifndef FRENZYKV_SERIALIZER_HELPER_H
#define FRENZYKV_SERIALIZER_HELPER_H

#include <bit>
#include <string>
#include <cstddef>
#include <cstdint>
#include <concepts>

#include "frenzykv/types.h"

namespace frenzykv
{

auto big_endian_encode(::std::integral auto i)
{
    if constexpr (::std::endian::native == ::std::endian::big)
    {
        return i;
    }
    else if constexpr (::std::endian::native == ::std::endian::little)
    {
        return ::std::byteswap(i);
    }
    else // mixed endian
    {
        ::std::terminate();
        return {};
    }
}

template<::std::size_t intsize>
void append_encode_int_to(::std::integral auto i, ::std::string& dst)
{
    static ::std::array<char, intsize> buffer{};
    static_assert(sizeof(i) == intsize, "privided size and actual size mismatch in Serializer_helper.h: append_encode_int_to() function ");
    i = big_endian_encode(i);
    ::std::memcpy(buffer.data(), &i, sizeof(i));
    dst.append(buffer.data(), buffer.size());
}

template<::std::size_t intsize>
void encode_int_to(::std::integral auto i, bspan buffer)
{
    assert(buffer.size_bytes() == intsize);
    i = big_endian_encode(i);
    ::std::memcpy(buffer.data(), &i, intsize);
}
    
template<::std::size_t intsize>
void encode_int_to(::std::integral auto i, ::std::span<char> buffer)
{
    // forward to another function, must not to byteswap
    encode_int_to<intsize>(i, ::std::as_writable_bytes(buffer));
}

} // namespace frenzykv

#endif
