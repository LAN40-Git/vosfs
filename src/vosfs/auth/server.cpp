#include "auth.pb.h"
#include "vosfs/auth/server.hpp"

auto vosfs::auth::AuthServer::create(uint16_t port) -> kosio::async::Task<Result<AuthServer>> {
    auto has_engine = detail::SQLiteEngine::create(DB_PATH);
    if (!has_engine) {
        co_return std::unexpected{has_engine.error()};
    }
    auto engine = std::move(has_engine.value());

    auto has_rpc_server = co_await rpc::RpcProvider::create(port);
    if (!has_rpc_server) {
        co_return std::unexpected{has_rpc_server.error()};
    }
    auto rpc_server = std::move(has_rpc_server.value());
    co_return AuthServer{std::move(engine), std::move(rpc_server)};
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

    if (request.name().empty() || request.hashed_password().empty()) {

    }

    auto& name = request.name();
    auto& hashed_password = request.hashed_password();
    auto role = request.role();
    auto status = Status::kEnabled;
    auto create_time = kosio::util::current_ms();
    auto modify_time = create_time;

    std::string_view sql = R"(
        INSERT INTO user (
            name, hashed_password, role, status, create_time, modify_time, last_login_time, last_login_ip
        ) VALUES (?, ?, ?, ?, ?, ?, 0, 'null');
    )";


}
