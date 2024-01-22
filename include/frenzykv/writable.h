#ifndef FRENZYKV_WRITABLE_H
#define FRENZYKV_WRITABLE_H

#include "koios/expected.h"
#include "toolpex/move_only.h"

#include <string_view>

namespace frenzykv
{

class seq_writable : public toolpex::move_only
{
public:
    seq_writable() = default;
    virtual ~seq_writable() noexcept {}

    koios::task<size_t> 
    append(::std::string_view buffer) noexcept 
    {
        ::std::span<const char> temp{ buffer.begin(), buffer.end() };
        return append(as_bytes(temp));
    }

    virtual koios::task<size_t> append(::std::span<const ::std::byte> buffer) = 0;
    virtual koios::task<> sync() = 0;
    virtual koios::task<> flush() = 0;
    virtual koios::task<> close() = 0;
};

} // namespace frenzykv

#endif
