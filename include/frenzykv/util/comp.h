#ifndef FRENZYKV_COMP_H
#define FRENZYKV_COMP_H

#include <compare>
#include <span>

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
};

} // namespace frenzykv

#endif
