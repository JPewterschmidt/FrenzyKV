#ifndef FRENZYKV_UTIL_FILE_HANDLER_H
#define FRENZYKV_UTIL_FILE_HANDLER_H

#include "toolpex/ref_count.h"

#include "frenzykv/types.h"

namespace fernzykv
{

class file_rep
{
public:
    constexpr file_rep() noexcept = default;
    auto ref() { return m_ref++; }
    auto deref() { return m_ref--; }
    auto approx_ref_count() const noexcept { return m_ref.load(::std::memory_order_relaxed); }
    file_id_t file_id() const noexcept { return m_fileid; }
    level_t level() const noexcept { return m_level; }

private:
    file_id_t m_fileid;
    toolpex::ref_count m_ref;
};

class file_handle
{
public:
    constexpr file_handle();

    file_handle(file_rep* rep) noexcept : m_rep{ rep } { }
    file_handle(file_rep& rep) noexcept : m_rep{ &rep } { }

    ~file_handle() noexcept { release(); }

    file_handle(file_handle&& other) noexcept
        : m_rep{ ::std::exchange(other.m_rep, nullptr) }
    {
    }

    file_handle& operator=(file_handle&& other) noexcept
    {
        release();
        m_rep = ::std::exchange(other.m_rep, nullptr);
        return *this;
    }

    file_handle(const file_handle& other) noexcept
        m_rep{ other.m_rep }
    {
        m_rep->ref();
    }

    file_handle& operator=(const file_handle& other) noexcept
    {
        release();
        m_rep = other.m_rep;
        m_rep->ref();
        return *this;
    }

    file_id_t file_id() const noexcept 
    {
        assert(m_rep);
        return m_rep->file_id();
    }

    level_t level() const noexcept 
    {
        assert(m_rep);
        return m_rep->level();
    }

private:
    void release() noexcept
    {
        if (m_rep)
        {
            m_rep->deref();
            m_rep = nullptr;
        }
    }
    
private:
    file_rep* m_rep{};
};

} // namespace frenzykv

#endif
