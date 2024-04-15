#ifndef FRENZYKV_DB_H
#define FRENZYKV_DB_H

#include "frenzykv/frenzykv.h"
#include "frenzykv/write_batch.h"

#include "koios/task.h"

#include <system_error>

namespace frenzykv
{

class db_interface
{
public:
    virtual ~db_interface() noexcept {}
    virtual koios::task<size_t> write(write_batch batch) = 0;
    virtual koios::task<size_t> write(const_bspan key, const_bspan value) { return write({key, value}); }
    virtual koios::task<size_t> remove_from_db(const_bspan key)
    {
        write_batch b;
        b.remove_from_db(key);
        return write(::std::move(b));
    }

    virtual koios::task<::std::optional<kv_entry>> get(const_bspan key)
    {
        ::std::error_code ec{};
        auto ret = co_await get(key, ec);
        if (ec) [[unlikely]] throw koios::exception{ec};
        co_return ret;
    }

    virtual koios::task<::std::optional<kv_entry>> 
    get(const_bspan key, ::std::error_code& ec_out) noexcept = 0;
};

} // namespace frenzykv

#endif
