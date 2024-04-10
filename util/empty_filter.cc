#include "frenzykv/db/filter.h"

namespace
{

class empty_filter : public filter_policy
{
public:
    constexpr ::std::string_view name() const noexcept override { return "empty filter"; }

    koios::task<bool> 
    append_new_filter([[maybe_unused]] ::std::span<const_bspan> key, 
                      [[maybe_unused]] ::std::string& dst) const override
    {
        co_return true;
    }

    koios::task<bool> 
    may_match([[maybe_unused]] const_bspan key, 
              [[maybe_unused]] ::std::string_view filter) const override
    {
        co_return true;
    }
};

} // annoymous namespace

namespace frenzykv
{

::std::unique_ptr<filter_policy> 
make_empty_filter()
{
    return ::std::make_unique<empty_filter>();
}

} // namespace frenzykv
