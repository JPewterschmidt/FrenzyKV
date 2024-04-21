#ifndef FRENZYKV_FILTER_H
#define FRENZYKV_FILTER_H

#include <string_view>
#include <string>
#include <memory>
#include <span>
#include "koios/task.h"
#include "frenzykv/frenzykv.h"

namespace frenzykv 
{

class filter_policy
{
public:
    virtual ~filter_policy() noexcept {}
    virtual ::std::string_view name() const noexcept = 0;
    virtual bool append_new_filter(::std::span<const_bspan> keys, ::std::string& dst) const = 0;
    virtual bool may_match(const_bspan key, ::std::string_view filter) const = 0;

    virtual bool append_new_filter(const_bspan key, ::std::string& dst) const
    {
        ::std::array<const_bspan, 1> buffer{ key };
        return append_new_filter(buffer, dst);
    }

    bool append_new_filter(::std::string_view sv, ::std::string& dst) const
    {
        return append_new_filter(::std::as_bytes(::std::span{sv}), dst);
    }
};

::std::unique_ptr<filter_policy> make_bloom_filter(size_t num_key_bits);
::std::unique_ptr<filter_policy> make_empty_filter();

} // namespace frenzykv

#endif
