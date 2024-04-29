#ifndef FRENZYKV_DB_REMOTE_H
#define FRENZYKV_DB_REMOTE_H

#include <memory>

#include "toolpex/unique_posix_fd.h"
#include "toolpex/ipaddress.h"

#include "koios/task.h"

#include "frenzykv/db.h"

namespace frenzykv
{

class db_remote : public db_interface
{
public:   
    db_remote(toolpex::ip_address::ptr ip, in_port_t port);
    koios::task<bool> connect();
    koios::task<bool> close();
    koios::task<bool> ping();

    virtual koios::task<::std::error_code> 
    insert(write_options opt, write_batch batch) override;

    virtual koios::task<::std::optional<kv_entry>> 
    get(const_bspan key, ::std::error_code& ec_out) noexcept override;
    
private:
    toolpex::ip_address::ptr m_server_ip;
    in_port_t m_server_port{};
    toolpex::unique_posix_fd m_sockfd;
};

} // namespace frenzykv

#endif
