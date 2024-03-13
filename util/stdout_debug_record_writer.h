#ifndef FRENZYKV_STDOUT_RECORD_WRITER_H
#define FRENZYKV_STDOUT_RECORD_WRITER_H

#include "koios/task.h"
#include "frenzykv/write_batch.h"

namespace frenzykv
{

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
