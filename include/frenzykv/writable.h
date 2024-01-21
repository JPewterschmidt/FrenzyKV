#ifndef FRENZYKV_WRITABLE_H
#define FRENZYKV_WRITABLE_H

#include "koios/task.h"
#include "toolpex/move_only.h"

#include <string_view>

namespace frenzykv
{

class seq_writable : public toolpex::move_only
{
public:
    seq_writable() = default;
    virtual ~seq_writable() noexcept {}

    koios::taskec 
    append(::std::string_view buffer) noexcept 
    {
        ::std::span<const char> temp{ buffer.begin(), buffer.end() };
        return append(as_bytes(temp));
    }

    virtual koios::taskec append(::std::span<const ::std::byte> buffer) noexcept = 0;
    virtual koios::taskec sync() noexcept = 0;
    virtual koios::taskec flush() noexcept = 0;
    virtual koios::taskec close() noexcept = 0;
};

} // namespace frenzykv

#endif
