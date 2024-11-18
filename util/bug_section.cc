#include <atomic>

#include "frenzykv/util/bug_section.h"

namespace frenzykv
{

static ::std::atomic_size_t g_bug_level{};

bug_section::bug_section()
{
    g_bug_level.fetch_add(1, ::std::memory_order_release);
}

bug_section::~bug_section() noexcept
{
    g_bug_level.fetch_sub(1, ::std::memory_order_release);
}

size_t bug_section::bug_level() noexcept
{
    return g_bug_level.load(::std::memory_order_acquire);
}

} // namespace frenzykv
