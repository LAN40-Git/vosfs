#include "auth.pb.h"
#include "vosfs/auth/server.hpp"
#include "vosfs/common/util/message_factory.hpp"

vosfs::auth::AuthServer::AuthServer(
    sqlite3* db, rpc::RpcServer rpc_server)
    : db_(db)
    , rpc_server_(std::move(rpc_server)) {}

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

void vosfs::auth::AuthServer::init() const {
    // 注册 RPC 服务
    using rpc::ServiceType;
    using rpc::MethodType;

    // put user
    rpc_server_->register_handler(ServiceType::kAuth, MethodType::kAuthPutUser,
        [this](std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
            co_return co_await this->handle_put_user_request(req_payload, resp_payload);
        });

    // get user
    rpc_server_->register_handler(ServiceType::kAuth, MethodType::kAuthGetUser,
        [this](std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
            co_return co_await this->handle_get_user_request(req_payload, resp_payload);
        });

    // update user name
    rpc_server_->register_handler(ServiceType::kAuth, MethodType::kAuthUpdateUserName,
        [this](std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
            co_return co_await this->handle_update_user_name_request(req_payload, resp_payload);
        });

    // update user password
    rpc_server_->register_handler(ServiceType::kAuth, MethodType::kAuthUpdateUserPassword,
        [this](std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
            co_return co_await this->handle_update_user_password_request(req_payload, resp_payload);
        });

    // update user role
    rpc_server_->register_handler(ServiceType::kAuth, MethodType::kAuthUpdateUserRole,
        [this](std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
            co_return co_await this->handle_update_user_role_request(req_payload, resp_payload);
        });

    // delet
    rpc_server_->register_handler(ServiceType::kAuth, MethodType::kAuthDeleteUser,
        [this](std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
            co_return co_await this->handle_delete_user_request(req_payload, resp_payload);
        });
}

auto vosfs::auth::AuthServer::run() const -> kosio::async::Task<void> {
    init();
    co_await rpc_server_->run();
}

auto vosfs::auth::AuthServer::shutdown() const -> kosio::async::Task<void> {
    co_await rpc_server_->shutdown();
}

auto vosfs::auth::AuthServer::handle_put_user_request(
    std::string_view req_payload,
    std::span<char> resp_payload) const -> kosio::async::Task<rpc::RpcResult> {
    PutUserRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
        co_return rpc::make_result(rpc::RpcResult::kMessageParseFailed);
    }

    auto& name = request.name();
    auto& hashed_password = request.hashed_password();
    auto role = request.role();
    auto status = kEnabled;
    auto create_time = static_cast<int64_t>(kosio::util::current_ms());
    auto modify_time = create_time;

    if (name.empty() || hashed_password.empty()) {
        co_return util::MessageFactory::make_put_user_response(resp_payload, false, "invalid name or password");
    }

    if (role != kAdmin && role != kUser) {
        co_return util::MessageFactory::make_put_user_response(resp_payload, false, "invalid role");
    }

    // 查询用户是否存在
    const char* sql = "SELECT 1 FROM user WHERE name = ? LIMIT 1;";
    sqlite3_stmt* stat;
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_put_user_response(resp_payload, false, "unknown error");
    }

    sqlite3_bind_text(stat, 1, name.data(), static_cast<int>(name.size()), SQLITE_STATIC);
    if (sqlite3_step(stat) == SQLITE_ROW) {
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_put_user_response(resp_payload, false, "repeated name");
    }
    sqlite3_reset(stat);
    sqlite3_clear_bindings(stat);

    // 尝试创建用户
    sql = R"(
        INSERT INTO user (
            name, hashed_password, role, status,
            create_time, modify_time, last_login_time, last_login_ip
        ) VALUES (?, ?, ?, ?, ?, ?, 0, '未知');
    )";
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_put_user_response(resp_payload, false, "unknown error");
    }

    sqlite3_bind_text(stat, 1, name.data(), static_cast<int>(name.size()), SQLITE_STATIC);
    sqlite3_bind_text(stat, 2, hashed_password.data(), static_cast<int>(hashed_password.size()), SQLITE_STATIC);
    sqlite3_bind_int(stat, 3, role);
    sqlite3_bind_int(stat, 4, status);
    sqlite3_bind_int64(stat, 5, create_time);
    sqlite3_bind_int64(stat, 6, modify_time);

    if (sqlite3_step(stat) != SQLITE_DONE) {
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_put_user_response(resp_payload, false, "unknown error");
    }

    sqlite3_finalize(stat);
    co_return util::MessageFactory::make_put_user_response(resp_payload, true, "register success");
}

auto vosfs::auth::AuthServer::handle_get_user_request(
    std::string_view req_payload,
    std::span<char> resp_payload) const -> kosio::async::Task<rpc::RpcResult> {
    GetUserRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
        co_return rpc::make_result(rpc::RpcResult::kMessageParseFailed);
    }

    auto name = std::string{request.name()};
    auto hashed_password = std::string{request.hashed_password()};

    if (request.name().empty() || request.hashed_password().empty()) {
        co_return util::MessageFactory::make_get_user_response(resp_payload, false, "invalid name or password");
    }

    // 查询用户是否存在
    bool exists{false};
    const char* sql = "SELECT 1 FROM user WHERE name = ? LIMIT 1;";
    sqlite3_stmt* stat;
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_get_user_response(resp_payload, false, "unknown error");
    }

    sqlite3_bind_text(stat, 1, name.data(), static_cast<int>(name.size()), SQLITE_STATIC);
    if (sqlite3_step(stat) == SQLITE_ROW) {
        exists = true;
    }
    sqlite3_reset(stat);
    sqlite3_clear_bindings(stat);

    // 用户不存在
    if (!exists) {
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_put_user_response(resp_payload, false, "user not exists");
    }

    sql = "SELECT uid, role, status, create_time FROM user WHERE name = ? AND hashed_password = ? LIMIT 1;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_put_user_response(resp_payload, false, "unknown error");
    }

    sqlite3_bind_text(stat, 1, name.data(), static_cast<int>(name.size()), SQLITE_STATIC);
    sqlite3_bind_text(stat, 2, hashed_password.data(), static_cast<int>(hashed_password.size()), SQLITE_STATIC);

    // 用户存在但密码错误
    if (sqlite3_step(stat) != SQLITE_ROW) {
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_put_user_response(resp_payload, false, "incorrect password");
    }

    // 账号密码都正确，返回用户信息
    auto uid = sqlite3_column_int64(stat, 0);
    auto role = sqlite3_column_int(stat, 1);
    auto status = sqlite3_column_int(stat, 2);
    auto create_time = sqlite3_column_int64(stat, 3);

    // 用户被禁用
    if (status == kDisabled) {
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_put_user_response(resp_payload, false, "user has been disabled");
    }
    sqlite3_reset(stat);
    sqlite3_clear_bindings(stat);

    // 更新用户登录时间和 IP
    sql = "UPDATE user SET last_login_time = ?, last_login_ip = ? WHERE uid = ?;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_get_user_response(resp_payload, false, "unknown error");
    }

    auto last_login_time = static_cast<int64_t>(kosio::util::current_ms());
    auto last_login_ip = rpc::get_current_session_ip();

    sqlite3_bind_int64(stat, 1, last_login_time);
    sqlite3_bind_text(stat, 2, last_login_ip.data(), static_cast<int>(last_login_ip.size()), SQLITE_STATIC);
    sqlite3_bind_int64(stat, 3, uid);

    // 更新失败
    if (sqlite3_step(stat) != SQLITE_DONE) {
        LOG_ERROR("failed to update data for user: {}-{}", uid, name);
    }

    // 返回用户信息
    sqlite3_finalize(stat);
    co_return util::MessageFactory::make_get_user_response(
        resp_payload,
        true,
        "login success",
        uid,
        std::move(name),
        role,
        create_time);
}

auto vosfs::auth::AuthServer::handle_update_user_name_request(
    std::string_view req_payload,
    std::span<char> resp_payload) const -> kosio::async::Task<rpc::RpcResult> {
    UpdateUserNameRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
        co_return rpc::make_result(rpc::RpcResult::kMessageParseFailed);
    }

    auto uid = static_cast<int64_t>(request.uid());
    auto& name = request.name();

    const char* sql = "UPDATE user SET name = ? WHERE uid = ?;";
    sqlite3_stmt* stat;
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_update_user_name_response(resp_payload, false, "unknown error");
    }

    sqlite3_bind_text(stat, 1, name.data(), static_cast<int>(name.size()), SQLITE_STATIC);
    sqlite3_bind_int64(stat, 2, uid);
    if (sqlite3_step(stat) != SQLITE_DONE) {
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_update_user_name_response(resp_payload, false, "modify failed");
    }

    sqlite3_finalize(stat);
    co_return util::MessageFactory::make_update_user_name_response(resp_payload, true, "modify success");
}

auto vosfs::auth::AuthServer::handle_update_user_password_request(
    std::string_view req_payload,
    std::span<char> resp_payload) const -> kosio::async::Task<rpc::RpcResult> {
    UpdateUserPasswordRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
        co_return rpc::make_result(rpc::RpcResult::kMessageParseFailed);
    }

    auto uid = static_cast<int64_t>(request.uid());
    auto& hashed_password = request.hashed_password();

    const char* sql = "UPDATE user SET hashed_password = ? WHERE uid = ?;";
    sqlite3_stmt* stat;
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_update_user_password_response(resp_payload, false, "unknown error");
    }

    sqlite3_bind_text(stat, 1, hashed_password.data(), static_cast<int>(hashed_password.size()), SQLITE_STATIC);
    sqlite3_bind_int64(stat, 2, uid);
    if (sqlite3_step(stat) != SQLITE_DONE) {
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_update_user_password_response(resp_payload, false, "modify failed");
    }

    sqlite3_finalize(stat);
    co_return util::MessageFactory::make_update_user_password_response(resp_payload, true, "modify success");
}

auto vosfs::auth::AuthServer::handle_update_user_role_request(
    std::string_view req_payload,
    std::span<char> resp_payload) const -> kosio::async::Task<rpc::RpcResult> {
    UpdateUserRoleRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
        co_return rpc::make_result(rpc::RpcResult::kMessageParseFailed);
    }

    auto uid = static_cast<int64_t>(request.uid());
    auto role = request.role();

    const char* sql = "UPDATE user SET role = ? WHERE uid = ?;";
    sqlite3_stmt* stat;
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_update_user_role_response(resp_payload, false, "unknown error");
    }

    sqlite3_bind_int(stat, 1, role);
    sqlite3_bind_int64(stat, 2, uid);
    if (sqlite3_step(stat) != SQLITE_DONE) {
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_update_user_role_response(resp_payload, false, "modify failed");
    }

    sqlite3_finalize(stat);
    co_return util::MessageFactory::make_update_user_role_response(resp_payload, true, "modify success");
}

auto vosfs::auth::AuthServer::handle_delete_user_request(
    std::string_view req_payload,
    std::span<char> resp_payload) const -> kosio::async::Task<rpc::RpcResult> {
    DeleteUserRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
        co_return rpc::make_result(rpc::RpcResult::kMessageParseFailed);
    }

    auto uid = static_cast<int64_t>(request.uid());
    auto& hashed_password = request.hashed_password();

    const char* sql = "DELETE FROM user WHERE uid = ? AND hashed_password = ?;";
    sqlite3_stmt* stat;
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_delete_user_response(resp_payload, false, "unknown error");
    }

    sqlite3_bind_int64(stat, 1, uid);
    sqlite3_bind_text(stat, 2, hashed_password.data(), static_cast<int>(hashed_password.size()), SQLITE_STATIC);
    if (sqlite3_step(stat) != SQLITE_DONE) {
        sqlite3_finalize(stat);
        co_return util::MessageFactory::make_delete_user_response(resp_payload, false, "delete failed");
    }

    sqlite3_finalize(stat);
    co_return util::MessageFactory::make_delete_user_response(resp_payload, true, "delete success");
}
