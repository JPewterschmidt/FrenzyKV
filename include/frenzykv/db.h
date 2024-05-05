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

    virtual koios::task<::std::error_code> 
    insert(write_batch batch, write_options opt = {}) = 0;

    virtual koios::task<::std::error_code> 
    insert(const_bspan key, const_bspan value, write_options opt = {}) 
    { 
        return insert({key, value}, ::std::move(opt)); 
    }

    virtual koios::task<::std::error_code> 
    remove_from_db(const_bspan key, write_options opt = {})
    {
        write_batch b;
        b.remove_from_db(key);

        // OK, wont be a couroutine issue.
        // because we move this batch in.
        return insert(::std::move(b), ::std::move(opt));
    }

    virtual koios::task<::std::optional<kv_entry>> 
    get(const_bspan key, read_options opt = {})
    {
        ::std::error_code ec{};
        auto ret = co_await get(key, ec, ::std::move(opt));
        if (ec) [[unlikely]] throw koios::exception{ec};
        co_return ret;
    }

    virtual koios::task<::std::optional<kv_entry>> 
    get(::std::string_view key, read_options opt = {})
    {
        ::std::span sp{ key.data(), key.size() };
        return get(::std::as_bytes(sp), ::std::move(opt));
    }

    virtual koios::task<::std::optional<kv_entry>> 
    get(const_bspan key, ::std::error_code& ec_out, read_options opt = {}) noexcept = 0;

    virtual koios::task<> close() = 0;
};

::std::unique_ptr<db_interface> open(toolpex::ip_address::ptr ip, in_port_t port);
::std::unique_ptr<db_interface> open(::std::filesystem::path path, options opt);

} // namespace frenzykv

#endif
