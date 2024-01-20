#ifndef FRENZYKV_IN_MEM_WRITABLE_H
#define FRENZYKV_IN_MEM_WRITABLE_H

#include "frenzykv/writable.h"
#include "frenzykv/inner_buffer.h"
#include "frenzykv/error_category.h"

namespace frenzykv
{

class in_mem_writable : public writable
{
public:
    virtual koios::task<::std::error_code> append(::std::span<const ::std::byte> buffer) noexcept override;
    virtual koios::task<::std::error_code> sync() noexcept override { co_return make_frzkv_ok(); }
    virtual koios::task<::std::error_code> flush() noexcept override { co_return make_frzkv_ok(); };
    virtual void close() noexcept override;

private:
    detials::buffer<> m_buffer;
};

} // namespace frenzykv

#endif
