#ifndef FRENZYKV_READABLE_H
#define FRENZYKV_READABLE_H

#include "toolpex/move_only.h"
#include "koios/task.h"

#include <span>
#include <cstddef>

namespace frenzykv
{

class seq_readable
{
public:
    virtual koios::task<::std::error_code>
    read(::std::span<::std::byte> dest) = 0;

    virtual ~seq_readable() noexcept {}
};

class random_readable : public seq_readable
{
public:
    virtual koios::task<::std::error_code> 
    read(::std::span<::std::byte>, size_t offset) const noexcept = 0;

    virtual koios::task<::std::error_code>
    read(::std::span<::std::byte> dest) override = 0;

    virtual ~random_readable() noexcept {}
};

class seq_readable_context
{
public:
    seq_readable_context() noexcept = default;
    seq_readable_context(seq_readable_context&&) noexcept = default;
    seq_readable_context(const seq_readable_context&) = default;
    seq_readable_context& operator = (seq_readable_context&&) noexcept = default;
    seq_readable_context& operator = (const seq_readable_context&) = default;

    size_t cursor() const noexcept { return m_cursor; }
    void has_read(size_t n) noexcept { m_cursor += n; }
    void reset() noexcept { m_cursor = 0; }
    void reset(const seq_readable_context& other) noexcept { m_cursor = other.m_cursor; }

private:
    size_t m_cursor{};
};

}

#endif
