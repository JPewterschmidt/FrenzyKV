#ifndef FRENZYKV_FLOCK_H
#define FRENZYKV_FLOCK_H

#include <sys/file.h>
#include <optional>
#include <cassert>

#include "koios/exceptions.h"
#include "koios/task_on_the_fly.h"

#include "toolpex/unique_posix_fd.h"

namespace frenzykv
{

class flock_exception : public koios::exception
{
public:
    using koios::exception::exception;
};

class flock_handler
{
protected:
    flock_handler(const toolpex::unique_posix_fd& fd)
        : m_fd{ &fd }, m_hold{ true }
    {
        if (!m_fd->valid()) [[unlikely]]
            throw flock_exception{"flock_exception: The `fd` is not a valid fd when construct a flock_handler obj."};
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
    } // TODO impl err handler

    // Both of these functions would test the validation of fd.
    // so you'd better to assume something by attribute [[assume]] when you calling them.
    friend ::std::optional<flock_handler> try_flock_shared(const toolpex::unique_posix_fd& fd);
    friend ::std::optional<flock_handler> try_flock_unique(const toolpex::unique_posix_fd& fd);

protected:
    const toolpex::unique_posix_fd* m_fd;
    bool m_hold;
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

} // namespace frenzykv

#endif
