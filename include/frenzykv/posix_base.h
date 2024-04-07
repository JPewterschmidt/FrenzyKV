#ifndef FRENZYKV_POSIX_BASE_H
#define FRENZYKV_POSIX_BASE_H

#include "toolpex/unique_posix_fd.h"
#include "util/flock.h"

namespace frenzykv
{
    class posix_base
    {
    public:
        posix_base(toolpex::unique_posix_fd fd)
            : m_fd{ ::std::move(fd) }, 
              m_flockhub{ m_fd }
        {
        }

        const toolpex::unique_posix_fd& fd() const noexcept { return m_fd; }
        auto acquire_shared_proc_flock() noexcept { return m_flockhub.acquire_shared(); }
        auto acquire_unique_proc_flock() noexcept { return m_flockhub.acquire_unique(); }

    protected:
        toolpex::unique_posix_fd m_fd;
        proc_flock_async_hub m_flockhub;       
    };
} // namespace frenzykv

#endif
