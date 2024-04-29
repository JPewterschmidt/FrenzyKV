#include "koios/iouring_awaitables.h"
#include "frenzykv/db/db_remote.h"

namespace frenzykv
{

db_remote::db_remote(toolpex::ip_address::ptr ip, in_port_t port)
    : m_server_ip{ ::std::move(ip) }, m_sockfd{ port }
{
    if (!m_server_ip) throw koios::exception{"db_remote ctor: null ipaddress ptr"};
}

koios::task<bool> db_remote::connect()
{
    try
    {
        m_sockfd = co_await koios::uring::connect_get_sock(m_server_ip, m_server_port);
    }
    catch (...)
    {
        co_return false;
    }
    co_return true;
}

koios::task<bool> db_remote::close()
{
    m_sockfd = {};
    co_return true;
}

koios::task<bool> db_remote::ping()
{
    // TODO
    co_return true;
}

koios::task<::std::error_code> 
db_remote::insert(write_options opt, write_batch batch)
{
    co_return {};
}

koios::task<::std::optional<kv_entry>> 
db_remote::get(const_bspan key, ::std::error_code& ec_out) noexcept
{
    co_return {};
}

} // namespace frenzykv
