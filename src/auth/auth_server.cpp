#include <jwt-cpp/jwt.h>
#include "authpb/auth.pb.h"
#include "vosfs/common/util/jwt.hpp"
#include "vosfs/auth/detail/config.hpp"
#include "../../include/vosfs/auth/status.hpp"
#include "vosfs/auth/auth_server.hpp"

auto vosfs::auth::AuthServer::create(
    std::string_view db_path,
    uint16_t port,
    std::string_view ip) -> Result<std::unique_ptr<AuthServer>> {
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

    return std::unique_ptr<AuthServer>(new AuthServer(db, port, ip));
}

void vosfs::auth::AuthServer::wait() {
    vrpc::TcpServerBuilder::options()
        .set_ip(ip_)
        .set_port(port_)
        .build()
        .register_method<RegisterUserRequest, RegisterUserResponse>(
            "user", "register",
            [this](const RegisterUserRequest& request) -> kosio::async::Task<RegisterUserResponse> {
                co_return co_await this->handle_register_user_request(request);
            })
        .register_method<DeleteUserRequest, DeleteUserResponse>(
            "user", "delete",
            [this](const DeleteUserRequest& request) -> kosio::async::Task<DeleteUserResponse> {
                co_return co_await this->handle_delete_user_request(request);
            })
        .register_method<LoginUserByNameRequest, LoginUserByNameResponse>(
            "user", "loginbyname",
            [this](const LoginUserByNameRequest& request) -> kosio::async::Task<LoginUserByNameResponse> {
                co_return co_await this->handle_login_user_by_user_name_request(request);
            })
        .wait();
}

auto vosfs::auth::AuthServer::handle_register_user_request(const RegisterUserRequest& request)
    -> kosio::async::Task<RegisterUserResponse> {
    ;
    RegisterUserResponse response;

    auto& user_name = request.user_name();
    auto& password = request.password();
    auto role = request.role();
    auto& admin_secret = request.admin_secret();
    auto status = User_Status_kEnabled;
    auto quota = detail::DEFAULT_USER_QUOTA_BYTES;
    auto create_time = kosio::util::current_ms();
    auto modify_time = create_time;
    auto last_access_time = create_time;
    auto cache = vrpc::get_server_cache();
    auto last_access_ip = std::visit([](auto&& ip) {
        return ip.to_string();
    }, cache->peer_addr.ip());

    if (user_name.empty() || password.empty()) {
        response.set_status_code(Status::kInvalidArgument);
        response.set_message("用户名或密码不能为空");
        co_return response;
    }

    if (!User_Role_IsValid(role)) {
        response.set_status_code(Status::kInvalidArgument);
        response.set_message("无效用户权限值");
        co_return response;
    }

    if (role == User_Role_kAdmin) {
        if (admin_secret != detail::DEFAULT_ADMIN_SECRET) {
            response.set_status_code(Status::kInvalidArgument);
            response.set_message("管理员密钥错误");
            co_return response;
        }
    }

    // 查询用户是否存在
    const char* sql = "SELECT 1 FROM user WHERE user_name = ? LIMIT 1;";
    sqlite3_stmt* stat;
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        response.set_status_code(Status::kInternal);
        response.set_message("服务器数据库错误，无法查询用户是否存在");
        co_return response;
    }

    sqlite3_bind_text(stat, 1, user_name.data(), static_cast<int>(user_name.size()), SQLITE_STATIC);
    {
        co_await mutex_.lock();
        std::lock_guard lock{mutex_, std::adopt_lock};
        if (sqlite3_step(stat) == SQLITE_ROW) {
            sqlite3_finalize(stat);
            response.set_status_code(Status::kInvalidArgument);
            response.set_message("用户名已存在");
            co_return response;
        }
    }
    sqlite3_reset(stat);

    // 尝试创建用户
    sql = R"(
        INSERT INTO user (
            user_name, avatar, password, role, status, quota,
            create_time, modify_time, last_access_time, last_access_ip
        ) VALUES (?, '', ?, ?, ?, ?, ?, ?, ?, ?);
    )";

    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        response.set_status_code(Status::kInternal);
        response.set_message("数据库 SQL 准备失败，无法创建用户");
        co_return response;
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

    {
        co_await mutex_.lock();
        std::lock_guard lock{mutex_, std::adopt_lock};
        if (sqlite3_step(stat) != SQLITE_DONE) {
            LOG_ERROR("{}", sqlite3_errmsg(db_));
            sqlite3_finalize(stat);
            response.set_status_code(Status::kInternal);
            response.set_message("数据库 SQL 执行失败，无法创建用户");
            co_return response;
        }
    }

    sqlite3_finalize(stat);
    response.set_status_code(Status::kOk);
    response.set_message("注册成功");
    co_return response;
}

auto vosfs::auth::AuthServer::handle_delete_user_request(const DeleteUserRequest& request)
    -> kosio::async::Task<DeleteUserResponse> {
    ;
    DeleteUserResponse response;

    auto& token = request.token();
    auto password = request.password();
    int64_t uid{};

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
        response.set_status_code(Status::kPermissionDenied);
        response.set_message("无效 Token");
        co_return response;
    }

    const char* sql = "DELETE FROM user WHERE uid = ? AND password = ?;";
    sqlite3_stmt* stat;
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        response.set_status_code(Status::kInternal);
        response.set_message("服务器数据库错误，无法删除用户");
        co_return response;
    }

    sqlite3_bind_int64(stat, 1, uid);
    sqlite3_bind_text(stat, 2, password.data(), static_cast<int>(password.size()), SQLITE_STATIC);

    {
        co_await mutex_.lock();
        std::lock_guard lock{mutex_, std::adopt_lock};
        if (sqlite3_step(stat) != SQLITE_DONE) {
            sqlite3_finalize(stat);
            response.set_status_code(Status::kInternal);
            response.set_message("服务器数据库错误，无法删除用户");
            co_return response;
        }
    }

    sqlite3_finalize(stat);
    response.set_status_code(Status::kOk);
    response.set_message("注销成功");
    co_return response;
}

auto vosfs::auth::AuthServer::handle_login_user_by_user_name_request(const LoginUserByNameRequest& request) -> kosio::async::Task<LoginUserByNameResponse> {
    ;
    LoginUserByNameResponse response;

    auto& user_name = request.user_name();
    auto& password = request.password();
    auto role = request.role();

    // 尝试登录
    const char* sql = "SELECT uid, avatar, status, quota, create_time FROM user WHERE user_name = ? AND password = ? AND role = ? LIMIT 1;";
    sqlite3_stmt* stat;
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        response.set_status_code(Status::kInternal);
        response.set_message("服务器数据库错误，无法查询用户");
        co_return response;
    }

    sqlite3_bind_text(stat, 1, user_name.data(), static_cast<int>(user_name.size()), SQLITE_STATIC);
    sqlite3_bind_text(stat, 2, password.data(), static_cast<int>(password.size()), SQLITE_STATIC);
    sqlite3_bind_int(stat, 3, role);
    {
        co_await mutex_.lock();
        std::lock_guard lock{mutex_, std::adopt_lock};
        if (sqlite3_step(stat) != SQLITE_ROW) {
            sqlite3_finalize(stat);
            response.set_status_code(Status::kInternal);
            response.set_message("用户名或密码错误");
            co_return response;
        }
    }

    auto uid = sqlite3_column_int64(stat, 0);
    auto* avatar = sqlite3_column_blob(stat, 1);
    auto avatar_bytes = sqlite3_column_bytes(stat, 1);
    auto status = sqlite3_column_int(stat, 2);
    auto quota = sqlite3_column_int64(stat, 3);
    auto create_time = sqlite3_column_int64(stat, 4);

    // 校验 status
    if (!User_Status_IsValid(status) ||
        status == User_Status_kDisabled ||
        status == User_Status_kDeleted) {
        sqlite3_finalize(stat);
        response.set_status_code(Status::kPermissionDenied);
        response.set_message("用户被禁用，请联系管理员");
        co_return response;
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

    auto decoded = jwt::decode(token);
    auto quota_str = decoded.get_payload_claim("quota").as_string();

    response.set_token(token);
    response.set_user_name(user_name);
    response.set_avatar(std::string(static_cast<const char*>(avatar), avatar_bytes));
    response.set_create_time(kosio::util::format_time(create_time));

    sqlite3_reset(stat);
    // 更新登录信息
    auto last_access_time = kosio::util::current_ms();
    auto cache = vrpc::get_server_cache();
    auto last_access_ip = std::visit([](auto&& ip) {
        return ip.to_string();
    }, cache->peer_addr.ip());

    sql = "UPDATE user SET last_access_time = ?, last_access_ip = ? WHERE uid = ?";
    if (sqlite3_prepare_v2(db_, sql, -1, &stat, nullptr) != SQLITE_OK) {
        LOG_ERROR("{}", sqlite3_errmsg(db_));
        sqlite3_finalize(stat);
        response.Clear();
        response.set_status_code(Status::kInternal);
        response.set_message("服务器数据库错误，无法更新用户数据");
        co_return response;
    }

    sqlite3_bind_int64(stat, 1, last_access_time);
    sqlite3_bind_text(stat, 2, last_access_ip.data(), static_cast<int>(last_access_ip.size()), SQLITE_STATIC);
    sqlite3_bind_int64(stat, 3, uid);
    {
        co_await mutex_.lock();
        std::lock_guard lock{mutex_, std::adopt_lock};
        if (sqlite3_step(stat) != SQLITE_DONE) {
            LOG_WARN("failed to update last access info of user-{}-{}", uid, user_name);
        }
    }
    sqlite3_finalize(stat);
    response.set_status_code(Status::kOk);
    response.set_message("登录成功");
    co_return response;
}
