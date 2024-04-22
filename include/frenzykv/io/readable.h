#ifndef FRENZYKV_READABLE_H
#define FRENZYKV_READABLE_H

#include "toolpex/move_only.h"
#include "koios/expected.h"

#include <span>
#include <memory>
#include <cstddef>

#include "frenzykv/io/file.h"

namespace frenzykv
{

class seq_readable : virtual public file
{
public:
    using uptr = ::std::unique_ptr<seq_readable>;

public:
    virtual koios::task<size_t>
    read(::std::span<::std::byte> dest) = 0;

    virtual ~seq_readable() noexcept {}
};

class random_readable : public seq_readable
{
public:
    virtual koios::task<size_t> 
    read(::std::span<::std::byte> dest, size_t offset) const = 0;

    virtual koios::task<size_t>
    read(::std::span<::std::byte> dest) override = 0;

    koios::task<size_t>
    read(::std::span<char> dest, size_t offset)
    {
        return read(::std::as_writable_bytes(dest), offset);
    }

    virtual koios::task<size_t>
    read(::std::span<char> dest)
    {
        return read(::std::as_writable_bytes(dest));
    }

    virtual ~random_readable() noexcept override {}
    virtual bool is_buffering() const override = 0;
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
    size_t has_read(size_t n) noexcept { m_cursor += n; return n; }
    void reset() noexcept { m_cursor = 0; }
    void reset(const seq_readable_context& other) noexcept { m_cursor = other.m_cursor; }

private:
    size_t m_cursor{};
};

}

#endif
