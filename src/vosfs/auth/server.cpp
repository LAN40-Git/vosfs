#include "vosfs/auth/server.hpp"

auto vosfs::auth::AuthServer::create(uint16_t port) -> kosio::async::Task<Result<AuthServer>> {
    auto ret = co_await rpc::RpcProvider::create(port);
    if (!ret) {
        co_return std::unexpected{ret.error()};
    }
    co_return AuthServer{std::move(ret.value())};
}

auto vosfs::auth::AuthServer::run() const -> kosio::async::Task<void> {
    co_await rpc_server_->run();
}

auto vosfs::auth::AuthServer::shutdown() const -> kosio::async::Task<void> {
    co_await rpc_server_->shutdown();
}
