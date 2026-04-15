#include "auth.pb.h"
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

auto vosfs::auth::AuthServer::handle_put_user_request(
    std::string_view req_payload,
    std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
    PutUserRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
        co_return rpc::make_result(rpc::RpcResult::kMessageParseFailed);
    }

    auto& name = request.name();
    auto& hashed_password = request.hashed_password();

    // TODO: 写入用户
}
