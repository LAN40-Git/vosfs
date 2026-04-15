#include "auth.pb.h"
#include "vosfs/auth/server.hpp"
#include "vosfs/common/util/message_factory.hpp"

vosfs::auth::AuthServer::AuthServer(
    sqlite3* db, rpc::RpcServer rpc_server)
    : db_(db)
    , rpc_server_(std::move(rpc_server)) {
    init();
}

vosfs::auth::AuthServer::~AuthServer() {
    if (db_) {
        sqlite3_close(db_);
    }
}

vosfs::auth::AuthServer::AuthServer(AuthServer&& other) noexcept
    : db_(other.db_)
    , rpc_server_(std::move(other.rpc_server_)) {
    other.db_ = nullptr;
}

auto vosfs::auth::AuthServer::operator=(AuthServer&& other) noexcept -> AuthServer& {
    if (this == &other) {
        return *this;
    }

    if (db_) {
        sqlite3_close(db_);
    }

    db_ = other.db_;
    other.db_ = nullptr;
    rpc_server_ = std::move(other.rpc_server_);
    return *this;
}

auto vosfs::auth::AuthServer::create(uint16_t port) -> kosio::async::Task<Result<AuthServer>> {
    sqlite3* db;
    if (sqlite3_open(DB_PATH.data(), &db) != SQLITE_OK) {
        co_return std::unexpected{make_error(Error::kCreateAuthServerFailed)};
    }

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS user (
            uid INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            hashed_password TEXT NOT NULL,
            role INTEGER NOT NULL,
            status INTEGER NOT NULL,
            create_time INTEGER NOT NULL,
            modify_time INTEGER NOT NULL,
            last_login_time INTEGER,
            last_login_ip TEXT
        );
    )";

    if (sqlite3_exec(db, sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
        co_return std::unexpected{make_error(Error::kCreateAuthServerFailed)};
    }

    auto has_rpc_server = co_await rpc::RpcProvider::create(port);
    if (!has_rpc_server) {
        co_return std::unexpected{has_rpc_server.error()};
    }
    auto rpc_server = std::move(has_rpc_server.value());
    co_return AuthServer{db, std::move(rpc_server)};
}

void vosfs::auth::AuthServer::init() {
    // 注册 RPC 服务
    using rpc::ServiceType;
    using rpc::MethodType;

    rpc_server_->register_handler(ServiceType::kAuth, MethodType::kAuthPutUser,
        [this](std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
            co_return co_await this->handle_put_user_request(req_payload, resp_payload);
        });
}

auto vosfs::auth::AuthServer::run() const -> kosio::async::Task<void> {
    if (db_ == nullptr) {
        co_return;
    }
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
        co_return util::MessageFactory::make_put_user_response(resp_payload, false, "invalid name or password");
    }

    auto& name = request.name();
    auto& hashed_password = request.hashed_password();
    auto role = request.role();
    auto status = Status::kEnabled;
    auto create_time = kosio::util::current_ms();
    auto modify_time = create_time;

    const char* check_sql = "SELECT 1 FROM user WHERE name = ? LIMIT 1;";
    sqlite3_stmt* check_stat;
    if (sqlite3_prepare_v2(db_, check_sql, -1, &check_stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(check_stat);
        co_return util::MessageFactory::make_put_user_response(resp_payload, false, "unknown error");
    }

    sqlite3_bind_text(check_stat, 1, name.data(), -1, SQLITE_STATIC);
    if (sqlite3_step(check_stat) == SQLITE_ROW) {
        sqlite3_finalize(check_stat);
        co_return util::MessageFactory::make_put_user_response(resp_payload, false, "repeated name");
    }
    sqlite3_finalize(check_stat);

    const char* put_sql = R"(
        INSERT INTO user (
            name, hashed_password, role, status,
            create_time, modify_time, last_login_time, last_login_ip
        ) VALUES (?, ?, ?, ?, ?, ?, 0, '未知');
    )";
    sqlite3_stmt* put_stat{nullptr};
    if (sqlite3_prepare_v2(db_, put_sql, -1, &put_stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(put_stat);
        co_return util::MessageFactory::make_put_user_response(resp_payload, false, "unknown error");
    }

    sqlite3_bind_text(put_stat, 1, name.data(), static_cast<int>(name.size()), SQLITE_STATIC);
    sqlite3_bind_text(put_stat, 2, hashed_password.data(), static_cast<int>(hashed_password.size()), SQLITE_STATIC);
    sqlite3_bind_int(put_stat, 3, static_cast<int>(role));
    sqlite3_bind_int(put_stat, 4, static_cast<int>(status));
    sqlite3_bind_int64(put_stat, 5, create_time);
    sqlite3_bind_int64(put_stat, 6, modify_time);

    if (sqlite3_step(put_stat) != SQLITE_DONE) {
        sqlite3_finalize(put_stat);
        co_return util::MessageFactory::make_put_user_response(resp_payload, false, "unknown error");
    }
    sqlite3_finalize(put_stat);

    co_return util::MessageFactory::make_put_user_response(resp_payload, true, "register success");
}
