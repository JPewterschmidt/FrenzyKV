#ifndef FRENZYKV_IN_MEM_RW_H
#define FRENZYKV_IN_MEM_RW_H

#include "frenzykv/writable.h"
#include "frenzykv/readable.h"
#include "frenzykv/inner_buffer.h"
#include "frenzykv/error_category.h"
#include "koios/coroutine_mutex.h"
#include "koios/generator.h"

#include <vector>

namespace frenzykv
{

class in_mem_rw final 
    : public seq_writable, 
      public random_readable, 
      public seq_readable_context
{
public:
    in_mem_rw(size_t block_size) 
        : m_block_size{ block_size }
    {
        m_blocks.emplace_back(m_block_size);
    }

    in_mem_rw(in_mem_rw&& other) noexcept
        : m_blocks{ ::std::move(other.m_blocks) }, 
          m_block_size{ other.m_block_size }
    {
    }

    virtual ~in_mem_rw() noexcept {}

    in_mem_rw& operator = (in_mem_rw&& other) noexcept
    {
        m_blocks = ::std::move(other.m_blocks);
        m_block_size = other.m_block_size;
        return *this;
    }

    koios::taskec append(::std::span<const ::std::byte> buffer) noexcept override;
    koios::taskec sync() noexcept override { co_return make_frzkv_ok(); }
    koios::taskec flush() noexcept override { co_return make_frzkv_ok(); }
    koios::taskec close() noexcept override { co_return make_frzkv_ok(); }

    koios::taskec
    read(::std::span<::std::byte> dest, size_t offset) const noexcept override;

    koios::taskec
    read(::std::span<::std::byte> dest) noexcept override;

private:
    koios::generator<::std::span<const ::std::byte>> 
    target_spans(size_t offset, size_t dest_size) const noexcept;

private:
    ::std::vector<detials::buffer<>> m_blocks;
    mutable koios::mutex m_mutex;
    size_t m_block_size;
};

} // namespace frenzykv

#endif
