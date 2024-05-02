#ifndef FRENZYKV_UTIL_UUID_H
#define FRENZYKV_UTIL_UUID_H

#include <cstring>
#include <string>
#include <string_view>
#include <compare>

#include "uuid/uuid.h"

namespace frenzykv
{

class uuid
{
public:
    uuid() noexcept;
    uuid(::std::string str);
    uuid(uuid&& other) noexcept;
    uuid& operator=(uuid&& other) noexcept;
    uuid(const uuid& other);
    uuid& operator=(const uuid& other);

    operator ::std::string_view() const { return to_string(); }
    ::std::string_view to_string() const { fill_string(); return m_str; } 

    // Lexicaographic order
    friend ::std::strong_ordering operator<=>(const uuid& lhs, const uuid& rhs) noexcept;

    bool operator != (const uuid& other) const noexcept
    {
        return ((*this) <=> other) != ::std::strong_ordering::equal;
    }

private:
    void fill_string() const;

private:
    ::uuid_t m_uuid{};
    mutable ::std::string m_str{};
};

} // namespace frenzykv

#endif
