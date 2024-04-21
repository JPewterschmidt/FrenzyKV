#ifndef FRENZYKV_SERIALIZER_HELPER_H
#define FRENZYKV_SERIALIZER_HELPER_H

#include <string>
#include <cstddef>
#include <cstdint>
#include <concepts>

#include "frenzykv/types.h"

namespace frenzykv
{

template<::std::size_t intsize>
void append_encode_int_to(::std::integral auto i, ::std::string& dst)
{
    // TODO: add endian check and byteswap
    static ::std::array<char, intsize> buffer{};
    static_assert(sizeof(i) == intsize, "privided size and actual size mismatch in Serializer_helper.h: append_encode_int_to() function ");
    ::std::memcpy(buffer.data(), &i, sizeof(i));
    dst.append(buffer.data(), buffer.size());
}

template<::std::size_t intsize>
void encode_int_to(::std::integral auto i, bspan buffer)
{
    // TODO: add endian check and byteswap
    assert(buffer.size_bytes() == intsize);
    ::std::memcpy(buffer.data(), &i, intsize);
}
    
template<::std::size_t intsize>
void encode_int_to(::std::integral auto i, ::std::span<char> buffer)
{
    // TODO: add endian check and byteswap
    encode_int_to<intsize>(i, ::std::as_writable_bytes(buffer));
}

} // namespace frenzykv

#endif
