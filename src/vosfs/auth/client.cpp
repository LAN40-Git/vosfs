#include "vosfs/auth/client.hpp"

auto vosfs::auth::AuthClient::create(std::string_view server_ip, uint16_t port) -> kosio::async::Task<Result<AuthClient>> {
    auto has_rpc_client = co_await rpc::RpcConsumer::create(server_ip, port);
    if (!has_rpc_client) {
        co_return std::unexpected{has_rpc_client.error()};
    }
    auto rpc_client = std::move(has_rpc_client.value());
    co_return AuthClient{rpc_client->server_addr(), std::move(rpc_client)};
}
