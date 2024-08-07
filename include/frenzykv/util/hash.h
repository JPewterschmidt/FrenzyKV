// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_HASH_H
#define FRENZYKV_HASH_H

#include <cstddef>
#include <cstdint>
#include <span>

namespace frenzykv
{

/*! \brief      The hash functor for binary contents
 *  \attention  This and the derived hash functor classes should be stateLESS
 */
class binary_hash_interface
{
public:
    virtual ::std::size_t operator()(::std::span<const ::std::byte> buffer) const noexcept = 0;
    virtual ::std::size_t operator()(::std::span<const ::std::byte> buffer, ::std::size_t hints) const noexcept = 0;

    ::std::size_t hash(::std::span<const ::std::byte> buffer) const noexcept 
    { 
        return (*this)(buffer); 
    }

    ::std::size_t hash(::std::span<const ::std::byte> buffer, ::std::size_t hints) const noexcept 
    { 
        return (*this)(buffer, hints); 
    }

    ::std::size_t hash(::std::span<const char> buffer) const noexcept 
    { 
        return this->hash(::std::as_bytes(buffer)); 
    }

    ::std::size_t hash(::std::span<const char> buffer, ::std::size_t hints) const noexcept 
    { 
        return this->hash(::std::as_bytes(buffer), hints); 
    }

    ::std::size_t operator()(::std::span<const char> buffer) const noexcept
    {
        return (*this)(::std::as_bytes(buffer));
    }

    ::std::size_t operator()(::std::span<const char> buffer, ::std::size_t hints) const noexcept
    {
        return (*this)(::std::as_bytes(buffer), hints);
    }
};

class std_bin_hash final : public binary_hash_interface
{
public:
    ::std::size_t operator()(::std::span<const ::std::byte> buffer) const noexcept override;
    ::std::size_t operator()(::std::span<const ::std::byte> buffer, ::std::size_t hints) const noexcept override;
};

class murmur_bin_hash_x64_128_xor_shift_to_64 final : public binary_hash_interface
{
public:
    ::std::size_t operator()(::std::span<const ::std::byte> buffer) const noexcept override;
    ::std::size_t operator()(::std::span<const ::std::byte> buffer, ::std::size_t hints) const noexcept override;
};

class murmur_bin_hash_x64_128_plus_to_64 final : public binary_hash_interface
{
public:
    ::std::size_t operator()(::std::span<const ::std::byte> buffer) const noexcept override;
    ::std::size_t operator()(::std::span<const ::std::byte> buffer, ::std::size_t hints) const noexcept override;
};

class murmur_bin_hash_x86_32 final : public binary_hash_interface
{
public:
    ::std::size_t operator()(::std::span<const ::std::byte> buffer) const noexcept override { return hash32(buffer); }
    ::std::size_t operator()(::std::span<const ::std::byte> buffer, ::std::size_t hints) const noexcept override { return hash32(buffer, hints); }
    
    uint32_t hash32(::std::span<const ::std::byte> buffer) const noexcept;
    uint32_t hash32(::std::span<const ::std::byte> buffer, ::std::size_t hints) const noexcept;
};

} // namespace frenzykv

#endif
