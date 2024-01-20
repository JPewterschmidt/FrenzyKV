#include "frenzykv/in_mem_writable.h"
#include "koios/task.h"
#include <stdexcept>
#include <cassert>
#include <vector>

namespace frenzykv
{

koios::task<::std::error_code> 
in_mem_writable::
append(::std::span<const ::std::byte> buffer) noexcept 
try
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
        buffer = buffer.subspan(last_buf.left());
    }
    while (!buffer.empty());

    co_return make_frzkv_ok();
}
catch (const ::std::out_of_range& e)
{
    co_return make_frzkv_out_of_range();
}
catch (...)
{
    co_return make_frzkv_exception_catched();
}

koios::task<::std::error_code>
in_mem_writable::
read(::std::span<::std::byte> dest, size_t offset) const noexcept
try
{
    const auto append_to_dest = [](auto& dest, const auto& src) noexcept
    {
        if (dest.empty()) return false;
        const size_t write_size = ::std::min(dest.size(), src.size());
        ::std::memcpy(dest.data(), src.data(), write_size);
        assert(write_size <= dest.size_bytes());
        dest = dest.subspan(0, write_size);
        return true;
    };

    const auto target_spans = [this](size_t offset, size_t dest_size) noexcept
    {
        size_t idx = offset / m_block_size;
        const size_t first_cursor = offset % m_block_size;
        size_t selected_bytes{};
        
        ::std::vector<::std::span<const ::std::byte>> result{};
        auto first_span = m_blocks[idx++]
            .valid_span()
            .subspan(first_cursor);
        selected_bytes += first_span.size_bytes();
        result.emplace_back(first_span);
        while (selected_bytes < dest_size && idx < m_blocks.size())
        {
            auto sp = m_blocks[idx++].valid_span();
            selected_bytes += sp.size_bytes();
        }

        return result;
    };

    auto lk = co_await m_mutex.acquire();

    for (auto s : target_spans(offset, dest.size_bytes()))
    {
        if (!append_to_dest(dest, s)) break;
    }

    co_return make_frzkv_ok();
}
catch (...)
{
    co_return make_frzkv_exception_catched();
}

} // namespace frenzykv
