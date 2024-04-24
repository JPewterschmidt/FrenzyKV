#include <vector>
#include <ranges>
#include <cassert>
#include <stdexcept>

#include "koios/task.h"

#include "frenzykv/io/in_mem_rw.h"

namespace rv = ::std::ranges::views;

namespace frenzykv
{

buffer<>&
in_mem_rw::
next_writable_buffer()
{
    auto& last_buf = m_blocks.back();
    if (last_buf.left() != 0)
        return last_buf;
    return m_blocks.emplace_back(m_block_size);
}

::std::span<::std::byte> 
in_mem_rw::
writable_span() noexcept 
{
    return next_writable_buffer().writable_span();
}

koios::task<> 
in_mem_rw::
commit(size_t wrote_len) noexcept 
{
    next_writable_buffer().commit(wrote_len);
    co_return;
}

koios::task<size_t> 
in_mem_rw::
append(::std::span<const ::std::byte> buffer)
{
    const size_t result = buffer.size_bytes();
    do
    {
        auto& last_buf = next_writable_buffer();
        const size_t appended = ::std::min(last_buf.left(), buffer.size());
        last_buf.append(buffer.subspan( 0, appended ));
        buffer = buffer.subspan(appended);
    }
    while (!buffer.empty());

    co_return result;
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
    const size_t write_size = ::std::min(dest.size_bytes(), src.size_bytes());

    ::std::memcpy(dest.data(), src.data(), write_size);
    assert(write_size <= dest.size_bytes());
    dest = dest.subspan(write_size);

    return write_size;
}

koios::task<size_t>
in_mem_rw::
read(::std::span<::std::byte> dest, size_t offset) const noexcept
{
    size_t result{};

    for (auto s : target_spans(offset, dest.size_bytes()))
    {
        const size_t readed = append_to_dest(dest, s);
        if (readed == 0) break;
        result += readed;
    }

    co_return result;
}

koios::task<size_t>
in_mem_rw::
read(::std::span<::std::byte> dest) noexcept
{
    const size_t ret = co_await read(dest, cursor());;
    has_read(ret);

    co_return ret;
}

uintmax_t in_mem_rw::
file_size() const noexcept
{
    uintmax_t result{};
    for (const auto& bf : m_blocks)
    {
        result += bf.size();
    }
    return result;
}

koios::task<size_t> in_mem_rw::
dump_to(seq_writable& file)
{
    size_t wrote{};
    for (const auto& buf : m_blocks 
        | rv::transform([](auto&& val){ return val.valid_span(); })
        | rv::filter([](auto&& sp){ return !sp.empty(); }))
    {
        size_t cur_wrote = co_await file.append(buf);
        if (cur_wrote == 0) co_return 0;
        wrote += cur_wrote;
    }
    assert(wrote == file_size());
    co_return wrote;
}

} // namespace frenzykv
