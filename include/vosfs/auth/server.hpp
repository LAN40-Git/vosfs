#pragma once
#include <sqlite3.h>
#include "vosfs/rpc/provider.hpp"

namespace vosfs::auth {
class AuthServer {
    static constexpr std::string_view DB_PATH = "vosfs.db";
private:
    explicit AuthServer(sqlite3* db, rpc::RpcServer rpc_server);

public:
    ~AuthServer();

    AuthServer(const AuthServer&) = delete;
    auto operator=(const AuthServer&) -> AuthServer& = delete;

    AuthServer(AuthServer&& other) noexcept;
    auto operator=(AuthServer&& other) noexcept -> AuthServer&;

public:
    [[REMEMBER_CO_AWAIT]]
    static auto create(uint16_t port) -> kosio::async::Task<Result<AuthServer>>;

public:
    void init() const; // 禁止放入构造函数
    auto run() const -> kosio::async::Task<void>;
    auto shutdown() const -> kosio::async::Task<void>;

private:
    [[REMEMBER_CO_AWAIT]]
    auto handle_put_user_request(std::string_view req_payload, std::span<char> resp_payload)
        const -> kosio::async::Task<rpc::RpcResult>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_get_user_request(std::string_view req_payload, std::span<char> resp_payload)
        const -> kosio::async::Task<rpc::RpcResult>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_update_user_name_request(std::string_view req_payload, std::span<char> resp_payload)
        const -> kosio::async::Task<rpc::RpcResult>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_update_user_password_request(std::string_view req_payload, std::span<char> resp_payload)
        const -> kosio::async::Task<rpc::RpcResult>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_update_user_role_request(std::string_view req_payload, std::span<char> resp_payload)
        const -> kosio::async::Task<rpc::RpcResult>;

private:
    sqlite3*       db_{nullptr};
    rpc::RpcServer rpc_server_;
};
} // namespace vosfs::auth