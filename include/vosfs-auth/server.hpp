#pragma once
#include <sqlite3.h>
#include <vrpc/server.hpp>
#include "vosfs/common/error.hpp"

namespace vosfs::auth {
class Server {
public:
    explicit Server(sqlite3* db, uint16_t port, std::string_view ip = "0.0.0.0")
        : db_(db)
        , rpc_server_(port, ip) {
        assert(db_);
    }

public:
    ~Server() {
        if (db_) {
            sqlite3_close(db_);
        }
    }

public:
    [[nodiscard]]
    static auto create(
        std::string_view db_path,
        uint16_t port,
        std::string_view ip = "0.0.0.0") -> Result<std::unique_ptr<Server>>;

public:
    void init();

    [[REMEMBER_CO_AWAIT]]
    auto wait() -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto shutdown() -> kosio::async::Task<void>;

private:
    [[REMEMBER_CO_AWAIT]]
    auto handle_register_user_request(std::string_view req_payload, std::span<char> resp_payload)
        -> kosio::async::Task<vrpc::InvokeResult>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_delete_user_request(std::string_view req_payload, std::span<char> resp_payload)
        -> kosio::async::Task<vrpc::InvokeResult>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_login_user_by_user_name_request(std::string_view req_payload, std::span<char> resp_payload)
        -> kosio::async::Task<vrpc::InvokeResult>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_login_user_by_email_request(std::string_view req_payload, std::span<char> resp_payload)
        -> kosio::async::Task<vrpc::InvokeResult>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_logout_user_request(std::string_view req_payload, std::span<char> resp_payload)
        -> kosio::async::Task<vrpc::InvokeResult>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_update_user_name_request(std::string_view req_payload, std::span<char> resp_payload)
        -> kosio::async::Task<vrpc::InvokeResult>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_update_user_avatar_request(std::string_view req_payload, std::span<char> resp_payload)
        -> kosio::async::Task<vrpc::InvokeResult>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_update_user_password_request(std::string_view req_payload, std::span<char> resp_payload)
        -> kosio::async::Task<vrpc::InvokeResult>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_update_user_role_request(std::string_view req_payload, std::span<char> resp_payload)
        -> kosio::async::Task<vrpc::InvokeResult>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_update_user_status_request(std::string_view req_payload, std::span<char> resp_payload)
        -> kosio::async::Task<vrpc::InvokeResult>;

    // [[REMEMBER_CO_AWAIT]]
    // auto handle_update_user_quota_request(std::string_view req_payload, std::span<char> resp_payload)
    //     -> kosio::async::Task<vrpc::InvokeResult>;

private:
    kosio::sync::SharedMutex mutex_;
    sqlite3*                 db_{nullptr};
    vrpc::Server             rpc_server_;
};
} // namespace vosfs::vosfs-auth