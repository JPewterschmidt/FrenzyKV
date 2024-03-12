#ifndef FRENZYKV_RECORD_WRITER_H
#define FRENZYKV_RECORD_WRITER_H

#include "frenzykv/concepts.h"
#include "frenzykv/write_batch.h"
#include "util/record_writer_wrapper.h"
#include "koios/task.h"
#include <tuple>

namespace frenzykv
{

class nop_record_writer
{
public:
    constexpr nop_record_writer() noexcept = default;
    constexpr ::std::suspend_never write([[maybe_unused]] const write_batch&) const noexcept { return {}; }
};

template<typename WritersTuple = ::std::tuple<nop_record_writer>>
class multi_dest_record_writer
{
public:
    constexpr multi_dest_record_writer() noexcept = default;
    multi_dest_record_writer(WritersTuple writers) noexcept
        : m_writers{ ::std::move(writers) }
    {
    }

    multi_dest_record_writer(multi_dest_record_writer&&) noexcept = default;
    multi_dest_record_writer& operator = (multi_dest_record_writer&&) noexcept = default;

    template<is_record_writer NewWriter>
    auto new_with(NewWriter&& nw) &&
    {
        auto t = ::std::tuple_cat(
            ::std::move(m_writers), 
            ::std::tuple{ ::std::forward<NewWriter>(nw) }
        );
        return multi_dest_record_writer<::std::remove_reference_t<decltype(t)>>{ ::std::move(t) };
    }

    koios::task<> write(const write_batch& batch)
    {
        co_await ::std::apply([&batch](auto&& ... writer_ptrs) -> koios::task<> {
            (co_await writer_ptrs.write(batch), ...);
        }, m_writers);
    }

private:
    WritersTuple m_writers{};   
};

class stdout_debug_record_writer
{
public:
    class awaitable
    {
    public:
        awaitable(const write_batch& b, size_t num = 0) noexcept : m_batch{ &b }, m_num{ num } {}
        constexpr bool await_ready() const noexcept { return true; }
        constexpr void await_suspend(::std::coroutine_handle<>) const noexcept {}
        void await_resume() const noexcept;

    private:
        const write_batch* m_batch{};
        size_t m_num{};
    };

public:
    stdout_debug_record_writer() noexcept;
    stdout_debug_record_writer(stdout_debug_record_writer&&) noexcept = default;
    awaitable write(const write_batch& batch) { return { batch, m_number }; }

private:
    size_t m_number{};
    static size_t s_number;
};

} // namespace frenzykv

#endif
