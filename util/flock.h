#ifndef FRENZYKV_FLOCK_H
#define FRENZYKV_FLOCK_H

#include <sys/file.h>
#include <optional>
#include <cassert>

#include "koios/exceptions.h"
#include "koios/async_awaiting_hub.h"
#include "koios/task_on_the_fly.h"

#include "toolpex/unique_posix_fd.h"

namespace frenzykv
{

class flock_exception : public koios::exception
{
public:
    using koios::exception::exception;
};

class flock_async_hub;

class flock_handler
{
protected:
    flock_handler(const toolpex::unique_posix_fd& fd)
        : m_fd{ &fd }, m_hold{ true }
    {
        if (!m_fd->valid()) [[unlikely]]
        {
            throw flock_exception{
                "flock_exception: The `fd` is not a valid fd "
                "when construct a flock_handler obj."
            };
        }
    }

public:
    constexpr flock_handler() noexcept
        : m_fd{}, m_hold{false}
    {
    }

    constexpr flock_handler(flock_handler&& other) noexcept
        : m_fd{ ::std::exchange(other.m_fd, nullptr) }, 
          m_hold{ ::std::exchange(other.m_hold, false) }
    {
    }

    constexpr flock_handler& operator = (flock_handler&& other) noexcept
    {
        unlock();
        m_fd = ::std::exchange(other.m_fd, nullptr);
        m_hold = ::std::exchange(other.m_hold, false);
        return *this;
    }

    ~flock_handler() noexcept { unlock(); }
    void unlock() noexcept 
    { 
        if (!m_hold && m_fd) return;
        int ret = ::flock(*m_fd, LOCK_UN | LOCK_NB); 
        assert(ret == 0);
        m_hold = false; 
        notify_hub();
    }

    // Both of these functions would test the validation of fd.
    // so you'd better to assume something by attribute [[assume]] when you calling them.
    friend ::std::optional<flock_handler> try_flock_shared(const toolpex::unique_posix_fd& fd);
    friend ::std::optional<flock_handler> try_flock_unique(const toolpex::unique_posix_fd& fd);

    friend class flock_async_hub;
    friend class flock_aw_base;
    friend class shared_flock_aw;
    friend class unique_flock_aw;

private:
    void notify_hub() noexcept;
    void set_hub(flock_async_hub& hub) noexcept { m_async_hub = &hub; }

protected:
    const toolpex::unique_posix_fd* m_fd;
    bool m_hold;
    flock_async_hub* m_async_hub{};
};

inline ::std::optional<flock_handler> 
try_flock_shared(const toolpex::unique_posix_fd& fd)
{
    if (!fd.valid()) [[unlikely]] throw flock_exception{ "invalid fd" };
    int ret = ::flock(fd, LOCK_SH | LOCK_NB);
    switch (ret)
    {
        case EWOULDBLOCK: return {};
        [[unlikely]] default: throw flock_exception{ret};
    }
    return flock_handler{fd};
}

inline ::std::optional<flock_handler> 
try_flock_unique(const toolpex::unique_posix_fd& fd)
{
    if (!fd.valid()) [[unlikely]] throw flock_exception{ "invalid fd" };
    int ret = ::flock(fd, LOCK_EX | LOCK_NB);
    switch (ret)
    {
        case EWOULDBLOCK: return {};
        [[unlikely]] default: throw flock_exception{ret};
    }
    return flock_handler{fd};
}

class flock_aw_base
{
public:
    flock_aw_base(flock_async_hub& h) noexcept : m_parent{h} {}
    void await_suspend(koios::task_on_the_fly t) noexcept;

protected:
    flock_async_hub& m_parent;
    ::std::optional<flock_handler> m_result{};
};

class shared_flock_aw : public flock_aw_base
{
public:
    using flock_aw_base::flock_aw_base;
    bool await_ready();
    flock_handler await_resume();  
};

class unique_flock_aw : public flock_aw_base
{
public:
    using flock_aw_base::flock_aw_base;
    bool await_ready();
    flock_handler await_resume();  
};

class flock_async_hub : public koios::async_awaiting_hub
{
public:
    flock_async_hub(const toolpex::unique_posix_fd& fd) noexcept
        : m_fd{ fd }
    {
    }

    const toolpex::unique_posix_fd& fd() const noexcept { return m_fd; }
    shared_flock_aw acquire_shared() noexcept { return {*this}; }
    unique_flock_aw acquire_unique() noexcept { return {*this}; }

private:
    const toolpex::unique_posix_fd& m_fd;
};

} // namespace frenzykv

#endif
