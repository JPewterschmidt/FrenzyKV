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

class proc_flock_async_hub;

/*! \brief the RAII handler of flock
 *  
 *  Unlike ::std::unique_lock, you can't regain the lock directly.
 *  If this was inited by a awaitable, when destruction, the next awaiting would be woke up.
 */
class proc_flock_handler
{
protected:
    proc_flock_handler(const toolpex::unique_posix_fd& fd)
        : m_fd{ &fd }, m_hold{ true }
    {
        if (!m_fd->valid()) [[unlikely]]
        {
            throw flock_exception{
                "flock_exception: The `fd` is not a valid fd "
                "when construct a proc_flock_handler obj."
            };
        }
    }

public:
    constexpr proc_flock_handler() noexcept
        : m_fd{}, m_hold{false}
    {
    }

    constexpr proc_flock_handler(proc_flock_handler&& other) noexcept
        : m_fd{ ::std::exchange(other.m_fd, nullptr) }, 
          m_hold{ ::std::exchange(other.m_hold, false) }
    {
    }

    constexpr proc_flock_handler& operator = (proc_flock_handler&& other) noexcept
    {
        unlock();
        m_fd = ::std::exchange(other.m_fd, nullptr);
        m_hold = ::std::exchange(other.m_hold, false);
        return *this;
    }

    ~proc_flock_handler() noexcept { unlock(); }

    /*! \brief  Release the ownership of the current flock.
     *
     *  Will wake the next awaiting task if there is one.
     */
    void unlock() noexcept 
    { 
        if (!m_hold || !m_fd) return;
        [[maybe_unused]] int ret = ::flock(*m_fd, LOCK_UN | LOCK_NB); 
        assert(ret == 0);
        m_hold = false; 
        notify_hub();
    }

    // Both of these functions would test the validation of fd.
    // so you'd better to assume something by attribute [[assume]] when you calling them.
    friend ::std::optional<proc_flock_handler> try_flock_shared(const toolpex::unique_posix_fd& fd);
    friend ::std::optional<proc_flock_handler> try_flock_unique(const toolpex::unique_posix_fd& fd);

    friend class proc_flock_async_hub;
    friend class proc_flock_aw_base;
    friend class shared_proc_flock_aw;
    friend class unique_proc_flock_aw;

private:
    void notify_hub() noexcept;
    void set_hub(proc_flock_async_hub& hub) noexcept { m_async_hub = &hub; }

protected:
    const toolpex::unique_posix_fd* m_fd;
    bool m_hold;
    proc_flock_async_hub* m_async_hub{};
};

/*! \brief  try acquire a flock on a specific fd.
 *  \param fd the file desctipter of the file you wanna lock.
 *  \attention flock only provide a process level advisory lock.
 *  \retval ::std::optional<proc_flock_handler> you are holding the lock now.
 *  \retval ::std::nullopt you aren't holding the lock.
 *
 *  flock only provide a process level advisory lock, 
 *  which means even though you hold a lock, other processes could do IO on it without any burden (advisory).
 *  Meanwhile, if a process has already hold a lock, then it acquires a new type lock on the same fd, 
 *  The old lock would be converted to new type, then return.
 *  The caller basically have no idea about wether the file which represented by fd has been locked or not.
 *
 *  BTW, this function would like to check the validation situation of `fd`, 
 *  before call the function you can use `[[assume(fd.valid())]]` to tell the compiler to do some optimization.
 *
 *  If you wanna a coroutine flock, you gonna see `proc_flock_async_hub`
 *
 *  \see `proc_flock_async_hub`
 *  \see `shared_proc_flock_aw`
 *  \see `unique_proc_flock_aw`
 */
inline ::std::optional<proc_flock_handler> 
try_flock_shared(const toolpex::unique_posix_fd& fd)
{
    if (!fd.valid()) [[unlikely]] throw flock_exception{ "invalid fd" };
    int ret = ::flock(fd, LOCK_SH | LOCK_NB);
    switch (ret)
    {
        case 0: [[assume(fd.valid())]]; 
                return proc_flock_handler{fd};
        case EWOULDBLOCK: return {};
        [[unlikely]] default: throw flock_exception{ret};
    }
}

/*! \brief  try acquire a flock on a specific fd.
 *  \param fd the file desctipter of the file you wanna lock.
 *  \attention flock only provide a process level advisory lock.
 *  \retval ::std::optional<proc_flock_handler> you are holding the lock now.
 *  \retval ::std::nullopt you aren't holding the lock.
 *
 *  flock only provide a process level advisory lock, 
 *  which means even though you hold a lock, other processes could do IO on it without any burden (advisory).
 *  Meanwhile, if a process has already hold a lock, then it acquires a new type lock on the same fd, 
 *  The old lock would be converted to new type, then return.
 *  The caller basically have no idea about wether the file which represented by fd has been locked or not.
 *
 *  BTW, this function would like to check the validation situation of `fd`, 
 *  before call the function you can use `[[assume(fd.valid())]]` to tell the compiler to do some optimization.
 *
 *  If you wanna a coroutine flock, you gonna see `proc_flock_async_hub`
 *
 *  \see `proc_flock_async_hub`
 *  \see `shared_proc_flock_aw`
 *  \see `unique_proc_flock_aw`
 */
inline ::std::optional<proc_flock_handler> 
try_flock_unique(const toolpex::unique_posix_fd& fd)
{
    if (!fd.valid()) [[unlikely]] throw flock_exception{ "invalid fd" };
    int ret = ::flock(fd, LOCK_EX | LOCK_NB);
    switch (ret)
    {
        case 0: [[assume(fd.valid())]]; 
                return proc_flock_handler{fd};
        case EWOULDBLOCK: return {};
        [[unlikely]] default: throw flock_exception{ret};
    }
}

class proc_flock_aw_base
{
public:
    proc_flock_aw_base(proc_flock_async_hub& h) noexcept : m_parent{h} {}
    void await_suspend(koios::task_on_the_fly t) noexcept;

protected:
    proc_flock_async_hub& m_parent;
    ::std::optional<proc_flock_handler> m_result{};
};

class shared_proc_flock_aw : public proc_flock_aw_base
{
public:
    using proc_flock_aw_base::proc_flock_aw_base;
    bool await_ready();
    proc_flock_handler await_resume();  
};

class unique_proc_flock_aw : public proc_flock_aw_base
{
public:
    using proc_flock_aw_base::proc_flock_aw_base;
    bool await_ready();
    proc_flock_handler await_resume();  
};

/*! \brief awaiting hub of flock
 *
 *  If you need coroutine flock, just like `koios::mutex` you ganna see this!
 *  Place this in somewhere holds the flock affair responsibility, 
 *  then you can `co_await` those member functions to acquire the lock you want.
 *  
 *  \attention  This facility should be used with associated fd 1 by 1. 
 *              Because hubs which associated with the same fd won't share the awaiting infomation.
 */
class proc_flock_async_hub : public koios::async_awaiting_hub
{
public:
    proc_flock_async_hub(const toolpex::unique_posix_fd& fd) noexcept
        : m_fd{ fd }
    {
    }

    const toolpex::unique_posix_fd& fd() const noexcept { return m_fd; }
    shared_proc_flock_aw acquire_shared() noexcept { return {*this}; }
    unique_proc_flock_aw acquire_unique() noexcept { return {*this}; }

private:
    const toolpex::unique_posix_fd& m_fd;
};

} // namespace frenzykv

#endif
