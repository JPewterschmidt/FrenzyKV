// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_COMP_H
#define FRENZYKV_COMP_H

#include <compare>
#include <span>
#include <string_view>

namespace frenzykv
{

class binary_comparator_interface
{
public:
    virtual ::std::strong_ordering 
    operator()(::std::span<const ::std::byte> lhs, 
               ::std::span<const ::std::byte> rhs) const noexcept = 0;

    auto operator()(::std::span<const char> lhs, 
                    ::std::span<const char> rhs) const noexcept
    {
        return (*this)(::std::as_bytes(lhs), ::std::as_bytes(rhs));
    }

    auto compare(::std::span<const ::std::byte> lhs, 
                 ::std::span<const ::std::byte> rhs) const noexcept
    {
        return (*this)(lhs, rhs);
    }

    auto compare(::std::span<const char> lhs, 
                 ::std::span<const char> rhs) const noexcept
    {
        return this->compare(::std::as_bytes(lhs), ::std::as_bytes(rhs));
    }
};

class memcmp_comparator final : public binary_comparator_interface
{
public:
    ::std::strong_ordering 
    operator()(::std::span<const ::std::byte> lhs, 
               ::std::span<const ::std::byte> rhs) const noexcept override;

    ::std::strong_ordering
    operator()(::std::string_view lhs, ::std::string_view rhs) const noexcept
    {
        return (*this)(::std::as_bytes(::std::span{lhs.data(), lhs.size()}), ::std::as_bytes(::std::span{rhs.data(), rhs.size()}));
    }
};

} // namespace frenzykv

#endif
