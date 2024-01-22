#include "frenzykv/in_mem_rw.h"
#include "koios/task.h"
#include <stdexcept>
#include <cassert>
#include <vector>

namespace frenzykv
{

koios::task<size_t> 
in_mem_rw::
append(::std::span<const ::std::byte> buffer)
{
    const auto next_writable_buffer = [this] -> decltype(auto) 
    {
        auto& last_buf = m_blocks.back();
        if (last_buf.left() != 0)
            return last_buf;
        return m_blocks.emplace_back(m_block_size);
    };

    auto lk = co_await m_mutex.acquire();
    do
    {
        auto& last_buf = next_writable_buffer();
        last_buf.append(buffer.subspan(
            0, ::std::min(last_buf.left(), buffer.size())
        ));
        buffer = buffer.subspan(last_buf.size());
    }
    while (!buffer.empty());

    co_return buffer.size_bytes();
}

koios::generator<::std::span<const ::std::byte>> 
in_mem_rw::
target_spans(size_t offset, size_t dest_size) const noexcept
{
    size_t idx = offset / m_block_size;
    const size_t first_cursor = offset % m_block_size;
    size_t selected_bytes{};
    
    auto first_span = m_blocks[idx++]
        .valid_span()
        .subspan(first_cursor);

    selected_bytes += first_span.size_bytes();
    co_yield first_span;

    while (selected_bytes < dest_size && idx < m_blocks.size())
    {
        auto sp = m_blocks[idx++].valid_span();
        selected_bytes += sp.size_bytes();
        co_yield sp;
    }
};

static size_t append_to_dest(auto& dest, const auto& src) noexcept
{
    if (dest.empty()) return 0;
    const size_t write_size = ::std::min(dest.size(), src.size());

    ::std::memcpy(dest.data(), src.data(), write_size);
    assert(write_size <= dest.size_bytes());
    dest = dest.subspan(write_size);

    return write_size;
}

koios::task<size_t>
in_mem_rw::
read(::std::span<::std::byte> dest, size_t offset) const 
{
    auto lk = co_await m_mutex.acquire();
    size_t result{};

    for (auto s : target_spans(offset, dest.size_bytes()))
    {
        const size_t wrote = append_to_dest(dest, s);
        if (wrote == 0) break;
        result += wrote;
    }

    co_return result;
}

koios::task<size_t>
in_mem_rw::
read(::std::span<::std::byte> dest) 
{
    seq_readable_context ctx = *this;

    const size_t ret = co_await read(dest, ctx.cursor());;
    ctx.has_read(ret);
    this->seq_readable_context::reset(ctx);

    co_return ret;
}

} // namespace frenzykv
