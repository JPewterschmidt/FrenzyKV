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
    virtual ~filter_policy() {}
    virtual ::std::string_view name() const noexcept = 0;
    virtual bool append_new_filter(::std::span<const_bspan> key, ::std::string& dst) const = 0;
    virtual bool may_match(const_bspan key, ::std::string_view filter) const = 0;
};

::std::unique_ptr<filter_policy> make_bloom_filter(size_t num_key_bits);
::std::unique_ptr<filter_policy> make_empty_filter();

} // namespace frenzykv

#endif
