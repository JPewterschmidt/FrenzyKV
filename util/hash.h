#ifndef FRENZYKV_HASH_H
#define FRENZYKV_HASH_H

#include <cstddef>
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
};

class std_bin_hash final : public binary_hash_interface
{
public:
    ::std::size_t operator()(::std::span<const ::std::byte> buffer) const noexcept override;
    ::std::size_t operator()(::std::span<const ::std::byte> buffer, ::std::size_t hints) const noexcept override;
};

class murmur_bin_hash final : public binary_hash_interface
{
public:
    ::std::size_t operator()(::std::span<const ::std::byte> buffer) const noexcept override;
    ::std::size_t operator()(::std::span<const ::std::byte> buffer, ::std::size_t hints) const noexcept override;
};

} // namespace frenzykv

#endif
