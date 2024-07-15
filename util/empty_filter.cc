// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "frenzykv/db/filter.h"

namespace frenzykv
{

namespace
{

class empty_filter : public filter_policy
{
public:
    constexpr ::std::string_view name() const noexcept override { return "empty filter"; }

    bool append_new_filter([[maybe_unused]] ::std::span<const_bspan> key, 
                           [[maybe_unused]] ::std::string& dst) const override
    {
        return true;
    }

    bool may_match([[maybe_unused]] const_bspan key, 
                   [[maybe_unused]] ::std::string_view filter) const override
    {
        return true;
    }
};

} // annoymous namespace

::std::unique_ptr<filter_policy> 
make_empty_filter()
{
    return ::std::make_unique<empty_filter>();
}

} // namespace frenzykv
