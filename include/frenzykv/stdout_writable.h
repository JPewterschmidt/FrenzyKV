#ifndef FRENZYKV_STDOUT_WRITABLE_H
#define FRENZYKV_STDOUT_WRITABLE_H

#include "frenzykv/writable.h"

namespace frenzykv
{

class stdout_writable : public seq_writable
{
public:
    virtual koios::exp_taskec<> append(::std::span<const ::std::byte> buffer) noexcept override;
    virtual koios::exp_taskec<> sync() noexcept override;
    virtual koios::exp_taskec<> flush() noexcept override;
    virtual koios::exp_taskec<> close() noexcept override; 
};

} // namespace frenzykv

#endif
