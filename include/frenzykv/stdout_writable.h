#ifndef FRENZYKV_STDOUT_WRITABLE_H
#define FRENZYKV_STDOUT_WRITABLE_H

#include "frenzykv/writable.h"

namespace frenzykv
{

class stdout_writable : public seq_writable
{
public:
    virtual koios::task<size_t> append(::std::span<const ::std::byte> buffer) override;
    virtual koios::task<> sync() noexcept override;
    virtual koios::task<> flush() noexcept override;
    virtual koios::task<> close() noexcept override; 
};

} // namespace frenzykv

#endif
