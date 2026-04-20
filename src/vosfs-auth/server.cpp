#include <jwt-cpp/jwt.h>
#include "authpb/auth.pb.h"
#include "vosfs/common/util/jwt.hpp"
#include "vosfs-auth/server.hpp"
#include "vosfs-auth/detail/rpc.hpp"
#include "vosfs-auth/detail/config.hpp"

auto vosfs::auth::Server::create(
    std::string_view db_path,
    uint16_t port,
    std::string_view ip) -> Result<std::unique_ptr<Server>> {
    // 连接数据库
    sqlite3* db = nullptr;
    if (sqlite3_open(db_path.data(), &db) != SQLITE_OK) {
        return std::unexpected{make_error(Error::kDatabaseConnectionFailed)};
    }

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS user (
            uid INTEGER PRIMARY KEY AUTOINCREMENT,
            user_name TEXT NOT NULL UNIQUE,
            avatar BLOB NOT NULL,
            email TEXT NOT NULL UNIQUE,
            phone TEXT NOT NULL UNIQUE,
            password TEXT NOT NULL,
            role INTEGER NOT NULL,
            status INTEGER NOT NULL,
            quota INTEGER NOT NULL,
            create_time INTEGER NOT NULL,
            modify_time INTEGER NOT NULL,
            last_access_time INTEGER,
            last_access_ip TEXT
        );
    )";

    char* err_msg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        LOG_ERROR("{}", err_msg);
        sqlite3_close(db);
        return std::unexpected{make_error(Error::kDatabaseQueryFailed)};
    }

    return std::unique_ptr<Server>(new Server(db, port, ip));
}

void vosfs::auth::Server::init() {
    using detail::ServiceType;
    using detail::InvokeType;

    rpc_server_.register_invoke(ServiceType::kUser, InvokeType::kRegisterUser,
        [this](std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<vrpc::InvokeResult> {
            co_return co_await this->handle_register_user_request(req_payload, resp_payload);
    });
}

auto vosfs::auth::Server::wait() -> kosio::async::Task<void> {
    co_await rpc_server_.wait();
}

auto vosfs::auth::Server::shutdown() -> kosio::async::Task<void> {
    co_await rpc_server_.shutdown();
}

auto vosfs::auth::Server::handle_register_user_request(
    std::string_view req_payload,
    std::span<char> resp_payload) -> kosio::async::Task<vrpc::InvokeResult> {
    RegisterUserRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
        co_return vrpc::make_result(vrpc::StatusCode::kUnknown);
    }

    auto& user_name = request.user_name();
    auto& password = request.password();
    auto role = request.role();
    auto& admin_secret = request.admin_secret();
    auto status = User_Status_kEnabled;
    auto quota = detail::DEFAULT_USER_QUOTA_BYTES;
    auto create_time = kosio::util::current_ms();
    auto modify_time = create_time;
    auto last_access_time = create_time;
    auto context = vrpc::get_context();
    auto last_access_ip = std::visit([](auto&& ip) {
        return ip.to_string();
    }, context->addr.ip());

    if (user_name.empty() || password.empty() || !User_Role_IsValid(role)) {
        co_return vrpc::make_result(vrpc::StatusCode::kInvalidArgument);
    }

    if (role == User_Role_kAdmin) {
        if (admin_secret != detail::DEFAULT_ADMIN_SECRET) {
            co_return vrpc::make_result(vrpc::StatusCode::kInvalidArgument);
        }
    }

    // 查询用户是否存在
    const char* sql = "SELECT 1 FROM user WHERE user_name = ? LIMIT 1;";
    sqlite3_stmt* stat;
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        co_return vrpc::make_result(vrpc::StatusCode::kUnknown);
    }

    sqlite3_bind_text(stat, 1, user_name.data(), static_cast<int>(user_name.size()), SQLITE_STATIC);
    co_await mutex_.lock_shared();
    if (sqlite3_step(stat) == SQLITE_ROW) {
        co_await mutex_.unlock_shared();
        sqlite3_finalize(stat);
        co_return vrpc::make_result(vrpc::StatusCode::kAlreadyExists);
    }
    co_await mutex_.unlock_shared();
    sqlite3_reset(stat);
    sqlite3_clear_bindings(stat);

    // 尝试创建用户
    sql = R"(
        INSERT INTO user (
            user_name, avatar, email, phone, password, role, status, quota,
            create_time, modify_time, last_access_time, last_access_ip
        ) VALUES (?, '', '', '', ?, ?, ?, '', ?, ?, ?, ?);
    )";

    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        co_return vrpc::make_result(vrpc::StatusCode::kUnknown);
    }

    sqlite3_bind_text(stat, 1, user_name.data(), static_cast<int>(user_name.size()), SQLITE_STATIC);
    sqlite3_bind_text(stat, 2, password.data(), static_cast<int>(password.size()), SQLITE_STATIC);
    sqlite3_bind_int(stat, 3, role);
    sqlite3_bind_int(stat, 4, status);
    sqlite3_bind_int64(stat, 5, static_cast<int64_t>(quota));
    sqlite3_bind_int64(stat, 6, create_time);
    sqlite3_bind_int64(stat, 7, modify_time);
    sqlite3_bind_int64(stat, 8, last_access_time);
    sqlite3_bind_text(stat, 9, last_access_ip.data(), static_cast<int>(last_access_ip.size()), SQLITE_STATIC);

    co_await mutex_.lock();
    if (sqlite3_step(stat) != SQLITE_DONE) {
        co_await mutex_.unlock();
        sqlite3_finalize(stat);
        co_return vrpc::make_result(vrpc::StatusCode::kUnknown);
    }
    co_await mutex_.unlock();

    sqlite3_finalize(stat);
    co_return vrpc::make_result(vrpc::StatusCode::kOk);
}

auto vosfs::auth::Server::handle_delete_user_request(
    std::string_view req_payload,
    std::span<char> resp_payload) -> kosio::async::Task<vrpc::InvokeResult> {
    DeleteUserRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
        co_return vrpc::make_result(vrpc::StatusCode::kUnknown);
    }

    auto& token = request.token();
    auto password = request.password();
    int64_t uid{};
    User_Role role{};

    // 校验 token
    try {
        auto decoded = jwt::decode(token);

        auto verifier = jwt::verify()
            .with_issuer(util::DEFAULT_JWT_ISSUER)
            .allow_algorithm(jwt::algorithm::hs256{util::DEFAULT_JWT_ISSUER_SECRET});

        verifier.verify(decoded);

        std::string uid_str = decoded.get_payload_claim("uid").as_string();
        uid = std::stoull(uid_str);
    } catch (...) {
        co_return vrpc::make_result(vrpc::StatusCode::kPermissionDenied);
    }

    const char* sql = "DELETE FROM user WHERE uid = ? AND password = ?;";
    sqlite3_stmt* stat;
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        co_return vrpc::make_result(vrpc::StatusCode::kUnknown);
    }

    sqlite3_bind_int64(stat, 1, uid);
    sqlite3_bind_text(stat, 2, password.data(), static_cast<int>(password.size()), SQLITE_STATIC);

    co_await mutex_.lock();
    if (sqlite3_step(stat) != SQLITE_DONE) {
        co_await mutex_.unlock();
        sqlite3_finalize(stat);
        co_return vrpc::make_result(vrpc::StatusCode::kUnknown);
    }
    co_await mutex_.unlock();

    sqlite3_finalize(stat);
    co_return vrpc::make_result(vrpc::StatusCode::kOk);
}

auto vosfs::auth::Server::handle_login_user_by_user_name_request(
    std::string_view req_payload,
    std::span<char> resp_payload) -> kosio::async::Task<vrpc::InvokeResult> {
    LoginUserByNameRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
        co_return vrpc::make_result(vrpc::StatusCode::kUnknown);
    }

    auto& user_name = request.user_name();
    auto& password = request.password();
    auto role = request.role();

    // 尝试登录
    const char* sql = "SELECT uid, avatar, email, phone, status, quota, create_time FROM user WHERE user_name = ? AND password = ? AND role = ? LIMIT 1;";
    sqlite3_stmt* stat;
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        co_return vrpc::make_result(vrpc::StatusCode::kUnknown);
    }

    sqlite3_bind_text(stat, 1, user_name.data(), static_cast<int>(user_name.size()), SQLITE_STATIC);
    sqlite3_bind_text(stat, 2, password.data(), static_cast<int>(password.size()), SQLITE_STATIC);
    sqlite3_bind_int(stat, 3, role);
    co_await mutex_.lock_shared();
    if (sqlite3_step(stat) != SQLITE_ROW) {
        // 登录失败
        co_await mutex_.unlock_shared();
        sqlite3_finalize(stat);
        co_return vrpc::make_result(vrpc::StatusCode::kInvalidArgument);
    }
    co_await mutex_.unlock_shared();
    sqlite3_reset(stat);
    sqlite3_clear_bindings(stat);

    auto uid = sqlite3_column_int64(stat, 0);
    auto* avatar = sqlite3_column_blob(stat, 1);
    auto avatar_bytes = sqlite3_column_bytes(stat, 1);
    auto* email = sqlite3_column_text(stat, 2);
    auto email_bytes = sqlite3_column_bytes(stat, 2);
    auto* phone = sqlite3_column_text(stat, 3);
    auto phone_bytes = sqlite3_column_bytes(stat, 3);
    auto status = sqlite3_column_int(stat, 4);
    auto quota = sqlite3_column_int64(stat, 5);
    auto create_time = sqlite3_column_int64(stat, 6);

    // 校验 status
    if (!User_Status_IsValid(status) ||
        status == User_Status_kDisabled ||
        status == User_Status_kDeleted) {
        co_return vrpc::make_result(vrpc::StatusCode::kPermissionDenied);
    }

    // 颁发 token
    auto token = jwt::create()
        .set_type("JWS")
        .set_issuer(util::DEFAULT_JWT_ISSUER)
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds(util::DEFAULT_TOKEN_VALID_TIME_IN_SECOND))
        .set_payload_claim("uid", jwt::claim(std::to_string(uid)))
        .set_payload_claim("role", jwt::claim(std::to_string(role)))
        .set_payload_claim("quota", jwt::claim(std::to_string(quota)))
        .sign(jwt::algorithm::hs256{util::DEFAULT_JWT_ISSUER_SECRET});

    // 生成回复
    LoginUserByNameResponse response;
    response.set_token(std::move(token));
    response.set_user_name(user_name);
    response.set_avatar(std::string(static_cast<const char*>(avatar), avatar_bytes));
    response.set_email(std::string(reinterpret_cast<const char*>(email), email_bytes));
    response.set_phone(std::string(reinterpret_cast<const char*>(phone), phone_bytes));
    response.set_create_time(kosio::util::format_time(create_time));

    // 更新登录信息
    auto last_access_time = kosio::util::current_ms();
    auto context = vrpc::get_context();
    auto last_access_ip = std::visit([](auto&& ip) {
        return ip.to_string();
    }, context->addr.ip());

    sql = "UPDATE user SET last_access_time = ?, last_access_ip = ? WHERE uid = ?";
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        co_return vrpc::make_result(vrpc::StatusCode::kUnknown);
    }

    sqlite3_bind_int64(stat, 1, last_access_time);
    sqlite3_bind_text(stat, 2, last_access_ip.data(), static_cast<int>(last_access_ip.size()), SQLITE_STATIC);
    sqlite3_bind_int64(stat, 3, uid);
    co_await mutex_.lock();
    if (sqlite3_step(stat) != SQLITE_DONE) {
        LOG_WARN("failed to update last access info of user-{}-{}", uid, user_name);
    }
    co_await mutex_.unlock();
    sqlite3_finalize(stat);
    co_return vrpc::make_result(response, resp_payload);
}
