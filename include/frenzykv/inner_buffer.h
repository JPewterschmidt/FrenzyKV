#ifndef FRENZYKV_INNER_BUFFER_H
#define FRENZYKV_INNER_BUFFER_H

#include <memory>
#include <cstddef>
#include <span>
#include <utility>

namespace frenzykv::detials
{

template<typename Alloc = ::std::allocator<char>>
class buffer
{
public:
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
    }

    ~buffer() noexcept
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
        m_alloc = ::std::move(other.m_alloc);
        m_storage = ::std::exchange(other.m_storage, nullptr);
        m_capa = other.m_capa;
        m_left = other.m_left;

        return *this;
    }

    size_t capacity() const noexcept { return m_capa; }
    size_t left() const noexcept { return m_left; }
    bool full() const noexcept { return m_capa == m_left; }
    bool valid() const noexcept 
    { 
        return m_storage != nullptr && capacity() != 0; 
    }

    bool fit(size_t want_to_appen_nbytes) const noexcept
    {
        return m_left <= want_to_appen_nbytes;
    }

    bool append(::std::span<const ::std::byte> buffer) noexcept
    {
        bool result = fit(buffer.size_bytes());
        if (result)
        {
            char* beg = cursor();
            ::std::memcpy(beg, buffer.data(), buffer.size_bytes());
        }
        return result;
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

private:
    size_t size() const noexcept
    {
        return capacity() - left();
    }

    char* cursor() const noexcept
    {
        return m_storage + size();
    }

private:
    Alloc m_alloc;
    char* m_storage{};
    size_t m_capa;
    size_t m_left;
};

} // namespace frenzykv::detials

#endif
