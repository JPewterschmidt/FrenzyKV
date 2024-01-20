#ifndef FRENZYKV_STDOUT_WRITABLE_H
#define FRENZYKV_STDOUT_WRITABLE_H

#include "frenzykv/writable.h"

namespace frenzykv
{

class stdout_writable : public seq_writable
{
public:
    virtual koios::task<::std::error_code> append(::std::span<const ::std::byte> buffer) noexcept override;
    virtual koios::task<::std::error_code> sync() noexcept override;
    virtual koios::task<::std::error_code> flush() noexcept override;
    virtual void close() noexcept override {}
};

} // namespace frenzykv

#endif
