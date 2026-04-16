#pragma once
#include "vosfs/rpc/consumer.hpp"

namespace vosfs::auth {
class AuthClient {
private:
    explicit AuthClient(rpc::RpcClient rpc_client)
        : rpc_client_(std::move(rpc_client)) {}

public:
    [[nodiscard]]
    static auto create(std::string_view server_ip, uint16_t port) -> AuthClient;

private:
    rpc::RpcClient rpc_client_;
};
} // namespace vosfs::auth