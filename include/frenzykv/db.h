#ifndef FRENZYKV_DB_H
#define FRENZYKV_DB_H

#include <memory>
#include <system_error>

#include "frenzykv/frenzykv.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/db/read_write_options.h"
#include "frenzykv/options.h"

#include "toolpex/ipaddress.h"
#include "koios/task.h"

namespace frenzykv
{

class db_interface
{
public:
    virtual ~db_interface() noexcept {}
    virtual koios::task<::std::error_code> insert(write_options opt, write_batch batch) = 0;
    virtual koios::task<::std::error_code> insert(write_options opt, const_bspan key, const_bspan value) 
    { 
        return insert(::std::move(opt), {key, value}); 
    }

    virtual koios::task<::std::error_code> remove_from_db(write_options opt, const_bspan key)
    {
        write_batch b;
        b.remove_from_db(key);

        // OK, wont be a couroutine issue.
        // because we move this batch in.
        return insert(::std::move(opt), ::std::move(b));
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

::std::unique_ptr<db_interface> open(toolpex::ip_address::ptr ip, in_port_t port);
::std::unique_ptr<db_interface> open(::std::filesystem::path path, options opt);

} // namespace frenzykv

#endif
