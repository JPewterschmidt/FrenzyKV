#include "frenzykv/in_mem_writable.h"

namespace frenzykv
{

koios::task<::std::error_code> 
in_mem_writable::append(::std::span<const ::std::byte> buffer) noexcept 
{
    if (m_buffer.append(buffer))
        co_return make_frzkv_ok();
    co_return make_frzkv_out_of_space();
}

void in_mem_writable::close() noexcept
{
    m_buffer = decltype(m_buffer){0};
}

} // namespace frenzykv
