#ifndef FRENZYKV_UTIL_BUG_SEGTION_H
#define FRENZYKV_UTIL_BUG_SEGTION_H

#include <cstddef>

namespace frenzykv
{

class bug_section
{
public:
    bug_section();
    ~bug_section() noexcept;

    bug_section(bug_section&&) noexcept = delete;
    bug_section(const bug_section&) = delete;
    bug_section& operator=(bug_section&&) noexcept = delete;
    bug_section& operator=(const bug_section&) = delete;

    static size_t bug_level() noexcept;
};

} // namesapce frenzykv

#endif
