#pragma once
#include "auth.pb.h"
#include <vrpc/client.hpp>

namespace vosfs::auth {
class AuthClient {
    struct UserSession {
        uint64_t    uid;
        std::string name;
    };
private:
    explicit AuthClient(const kosio::net::SocketAddr& server_addr, rpc::RpcClient rpc_client)
        : server_addr_(server_addr)
        , rpc_client_(std::move(rpc_client)) {}

public:
    [[nodiscard]]
    static auto create(std::string_view server_ip, uint16_t port) -> kosio::async::Task<Result<AuthClient>>;

public:
    [[REMEMBER_CO_AWAIT]]
    auto send_put_user_request(std::string_view name, std::string_view password, int role) -> kosio::async::Task<void>;
    auto handle_put_user_response(std::string_view resp_payload) -> kosio::async::Task<void>;
    auto send_get_user_request(std::string_view name, std::string_view password, int role) -> kosio::async::Task<void>;
    auto handle_get_user_response(std::string_view resp_payload) -> kosio::async::Task<void>;
    auto send_update_user_name_request(std::string_view name) -> kosio::async::Task<void>;
    auto handle_update_user_name_response(std::string_view resp_payload) -> kosio::async::Task<void>;
    auto send_update_user_password_request() -> kosio::async::Task<void>;
    auto handle_update_user_password_response(std::string_view resp_payload) -> kosio::async::Task<void>;
    auto send_update_user_role_request() -> kosio::async::Task<void>;
    auto handle_update_user_role_response(std::string_view resp_payload) -> kosio::async::Task<void>;
    auto send_delete_user_request() -> kosio::async::Task<void>;
    auto handle_delete_user_response(std::string_view resp_payload) -> kosio::async::Task<void>;

private:
    kosio::net::SocketAddr server_addr_;
    rpc::RpcClient         rpc_client_;

};
} // namespace vosfs::vosfs-auth