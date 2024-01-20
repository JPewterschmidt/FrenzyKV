#ifndef FRENZYKV_IN_MEM_RW_H
#define FRENZYKV_IN_MEM_RW_H

#include "frenzykv/writable.h"
#include "frenzykv/readable.h"
#include "frenzykv/inner_buffer.h"
#include "frenzykv/error_category.h"
#include "koios/coroutine_mutex.h"

#include <vector>

namespace frenzykv
{

class in_mem_rw : public writable, public readable
{
public:
    in_mem_rw(size_t block_size) noexcept
        : m_block_size{ block_size }
    {
    }

    in_mem_rw(in_mem_rw&& other) noexcept
        : m_blocks{ ::std::move(other.m_blocks) }, 
          m_block_size{ other.m_block_size }
    {
    }

    in_mem_rw& operator = (in_mem_rw&& other) noexcept
    {
        m_blocks = ::std::move(other.m_blocks);
        m_block_size = other.m_block_size;
        return *this;
    }

    virtual koios::task<::std::error_code> append(::std::span<const ::std::byte> buffer) noexcept override;
    virtual koios::task<::std::error_code> sync() noexcept override { co_return make_frzkv_ok(); }
    virtual koios::task<::std::error_code> flush() noexcept override { co_return make_frzkv_ok(); };
    virtual void close() noexcept override {}

    koios::task<::std::error_code> 
    read(::std::span<::std::byte> dest, size_t offset) const noexcept override;

private:
    ::std::vector<detials::buffer<>> m_blocks;
    mutable koios::mutex m_mutex;
    size_t m_block_size;
};

} // namespace frenzykv

#endif
