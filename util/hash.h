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
    virtual ::std::size_t hash(::std::span<const ::std::byte> buffer) noexcept = 0;
    virtual ::std::size_t hash(::std::span<const ::std::byte> buffer, ::std::size_t hints) noexcept = 0;

    ::std::size_t operator()(::std::span<const ::std::byte> buffer) noexcept
    {
        return this->hash(buffer);
    }

    ::std::size_t operator()(::std::span<const ::std::byte> buffer, ::std::size_t hints) noexcept
    {
        return this->hash(buffer, hints);
    }
};

class std_bin_hash final : public binary_hash_interface
{
public:
    ::std::size_t hash(::std::span<const ::std::byte> buffer) noexcept override;
    ::std::size_t hash(::std::span<const ::std::byte> buffer, ::std::size_t hints) noexcept override;
};

class murmur_bin_hash final : public binary_hash_interface
{
public:
    ::std::size_t hash(::std::span<const ::std::byte> buffer) noexcept override;
    ::std::size_t hash(::std::span<const ::std::byte> buffer, ::std::size_t hints) noexcept override;
};

} // namespace frenzykv

#endif
