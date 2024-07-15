// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_INNER_BUFFER_H
#define FRENZYKV_INNER_BUFFER_H

#include <memory>
#include <cstddef>
#include <span>
#include <utility>

#include "frenzykv/kvdb_deps.h"

namespace frenzykv
{

template<typename Alloc = ::std::allocator<::std::byte>>
class buffer
{
public:
    using allocator = Alloc;
    using pointer = typename ::std::allocator_traits<allocator>::pointer;
    using const_pointer = typename ::std::allocator_traits<allocator>::const_pointer;

public:
    constexpr buffer() noexcept = default;

    buffer(size_t capacity)
        : m_capa{ capacity }, m_left{ capacity }
    {
        if (m_capa == 0) 
        {
            m_storage = nullptr;
            return;
        }
        m_storage = ::std::allocator_traits<Alloc>::allocate(
            m_alloc, m_capa
        );
#ifdef FRENZYKV_DEBUG
        turncate();
#endif
    }

    buffer(const kvdb_deps& deps)
        : buffer{ deps.opt()->memory_page_bytes }
    {
    }

    ~buffer() noexcept { release(); }

    void release() noexcept
    {
        if (!valid()) return;
        ::std::allocator_traits<Alloc>::deallocate(
            m_alloc, m_storage, m_capa
        );
    }

    buffer(buffer&& other) noexcept
        : m_alloc{ ::std::move(other.m_alloc) }, 
          m_storage{ ::std::exchange(other.m_storage, nullptr) }, 
          m_capa{ other.m_capa }, 
          m_left{ other.m_left }
    {
    }

    buffer& operator = (buffer&& other) noexcept
    {
        release();
        m_alloc = ::std::move(other.m_alloc);
        m_storage = ::std::exchange(other.m_storage, nullptr);
        m_capa = other.m_capa;
        m_left = other.m_left;

        return *this;
    }

    bool empty() const noexcept { return size() == 0; }
    size_t capacity() const noexcept { return m_capa; }
    size_t left() const noexcept { return m_left; }
    size_t size() const noexcept { return capacity() - left(); }
    bool full() const noexcept { return left() == 0; }
    bool valid() const noexcept 
    { 
        return m_storage != nullptr && capacity() != 0; 
    }

    bool fit(size_t want_to_appen_nbytes) const noexcept
    {
        return m_left >= want_to_appen_nbytes;
    }

    bool append(::std::span<const ::std::byte> buffer) noexcept
    {
        bool result = fit(buffer.size_bytes());
        if (result)
        {
            const size_t buffer_sz = buffer.size_bytes();
            auto mem = writable_span();
            ::std::memcpy(mem.data(), buffer.data(), buffer_sz);
            [[assume(buffer_sz <= m_left)]];
            commit(buffer_sz);
        }
        return result;
    }

    /*! \return Available memory span which could be wrote.
     *
     *  \attention After write, you have to call `commit()` 
     *             to inform the buffer how many memory you wrote.
     *
     *  May be used to implement something meet the requirements of STL ostream.
     */
    ::std::span<::std::byte> writable_span() const noexcept { return { cursor(), left() }; }

    /*! \brief Commits how many bytes has been wrote 
     *         to the most recent returned writable span from `writable_span()`
     *
     *  \param wrote_len the number of bytes you have worten, 
     *                   must less equal than the size of writable span.
     *
     *  May be used to implement something meet the requirements of STL ostream.
     */
    void commit(size_t wrote_len) noexcept
    {
        assert(wrote_len <= m_left);
        m_left -= wrote_len;
    }

    void turncate() noexcept
    {
        m_left = capacity();
#ifdef FRENZYKV_DEBUG
        ::std::memset(m_storage, 0, capacity());
#endif
    }

    ::std::span<const ::std::byte> valid_span() const noexcept
    {
        return { reinterpret_cast<const ::std::byte*>(m_storage), size() };
    }

    ::std::span<const ::std::byte> whole_span() const noexcept
    {
        return { reinterpret_cast<const ::std::byte*>(m_storage), capacity() };
    }

    ::std::span<::std::byte> valid_span() noexcept
    {
        return { reinterpret_cast<::std::byte*>(m_storage), size() };
    }

    ::std::span<::std::byte> whole_span() noexcept
    {
        return { reinterpret_cast<::std::byte*>(m_storage), capacity() };
    }

private:
    ::std::byte* cursor() const noexcept
    {
        return reinterpret_cast<::std::byte*>(m_storage + size());
    }

private:
    allocator m_alloc;
    pointer m_storage{};
    size_t m_capa;
    size_t m_left;
};

class outbuf_adapter : public ::std::streambuf
{
public:
    outbuf_adapter(buffer<>& buf) : m_buf{ &buf } { update_state(); }

    void update_state()
    {
        auto mem = m_buf->writable_span();
        setp(reinterpret_cast<char*>(mem.data()), 
             reinterpret_cast<char*>(mem.data() + mem.size_bytes()));
    }

protected:
    // The two following functions means, 
    // user should sync or flush the buffer bu hands with those async methods above.
    virtual int_type overflow(int_type ch = traits_type::eof()) noexcept override
    {
        return traits_type::eof();
    }
    virtual int_type sync() noexcept override { return -1; }

    virtual ::std::streamsize xsputn(const char* s, ::std::streamsize n) noexcept override
    {
        if (m_buf->append(as_bytes(::std::span{s, static_cast<size_t>(n)})))
        {
            update_state();
            return n;
        }
        return 0;
    }

private:
    buffer<>* m_buf;
};

class inbuf_adapter : public ::std::streambuf
{
public:
    inbuf_adapter(buffer<>& buf) : m_buf{ &buf } { update_state(); }

    void update_state()
    {
        auto mem = m_buf->valid_span();
        setg(reinterpret_cast<char*>(mem.data()), 
             reinterpret_cast<char*>(mem.data()), 
             reinterpret_cast<char*>(mem.data() + mem.size_bytes()));
    }

protected:
    // The two following functions means, 
    // user should sync or flush the buffer bu hands with those async methods above.
    virtual int_type underflow() noexcept override { return traits_type::eof(); }
    virtual int_type sync() noexcept override { return -1; }

    virtual ::std::streamsize xsgetn(char* s, ::std::streamsize n) noexcept override
    {
        auto vspn = m_buf->valid_span();
        using size_type = decltype(vspn)::size_type;
        auto actual_copy_from = vspn.subspan(0, ::std::min(static_cast<size_type>(n), vspn.size_bytes()));
        ::std::memcpy(s, actual_copy_from.data(), actual_copy_from.size_bytes());
        return actual_copy_from.size_bytes();
    }

private:
    buffer<>* m_buf;
};

} // namespace frenzykv

#endif
