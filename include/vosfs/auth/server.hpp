#pragma once
#include "vosfs/rpc/provider.hpp"

namespace vosfs::auth {
class AuthServer {
private:
    explicit AuthServer(rpc::RpcServer rpc_server)
        : rpc_server_(std::move(rpc_server)) {}

public:
    [[REMEMBER_CO_AWAIT]]
    static auto create(uint16_t port) -> kosio::async::Task<Result<AuthServer>>;

public:
    auto run() const -> kosio::async::Task<void>;
    auto shutdown() const -> kosio::async::Task<void>;

private:


private:
    rpc::RpcServer rpc_server_;
};
} // namespace vosfs::auth