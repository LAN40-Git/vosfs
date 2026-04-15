#pragma once
#include "vosfs/rpc/provider.hpp"
#include "vosfs/auth/internal/sqlite_engine.hpp"

namespace vosfs::auth {
class AuthServer {
    static constexpr std::string_view DB_PATH = "vosfs.db";
private:
    explicit AuthServer(
        detail::SQLiteEngine&& engine, rpc::RpcServer rpc_server)
        : engine_(std::move(engine))
        , rpc_server_(std::move(rpc_server)) {}

public:
    [[REMEMBER_CO_AWAIT]]
    static auto create(uint16_t port) -> kosio::async::Task<Result<AuthServer>>;

public:
    auto run() const -> kosio::async::Task<void>;
    auto shutdown() const -> kosio::async::Task<void>;

private:
    [[REMEMBER_CO_AWAIT]]
    auto handle_put_user_request(std::string_view req_payload, std::span<char> resp_payload)
        -> kosio::async::Task<rpc::RpcResult>;

private:
    detail::SQLiteEngine engine_;
    rpc::RpcServer       rpc_server_;
};
} // namespace vosfs::auth