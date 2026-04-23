#pragma once
#include <sqlite3.h>
#include <vrpc/net/tcp/tcp_server.hpp>
#include "authpb/auth.pb.h"
#include "vosfs/common/error.hpp"

namespace vosfs::auth {
class AuthServer {
private:
    explicit AuthServer(sqlite3* db, uint16_t port, std::string_view ip = "0.0.0.0")
        : db_(db)
        , port_(port)
        , ip_(ip) {
        assert(db_);
    }

public:
    ~AuthServer() {
        if (db_) {
            sqlite3_close(db_);
        }
    }

public:
    [[nodiscard]]
    static auto create(
        std::string_view db_path,
        uint16_t port,
        std::string_view ip = "0.0.0.0") -> Result<std::unique_ptr<AuthServer>>;

public:
    void wait();

private:
    [[REMEMBER_CO_AWAIT]]
    auto handle_register_user_request(const RegisterUserRequest& request)
        -> kosio::async::Task<RegisterUserResponse>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_delete_user_request(const DeleteUserRequest& request)
        -> kosio::async::Task<DeleteUserResponse>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_login_user_by_user_name_request(const LoginUserByNameRequest& request)
        -> kosio::async::Task<LoginUserByNameResponse>;

    // [[REMEMBER_CO_AWAIT]]
    // auto handle_login_user_by_email_request(std::string_view req_payload, std::span<char> resp_payload)
    //     -> kosio::async::Task<vrpc::InvokeResult>;
    //
    // [[REMEMBER_CO_AWAIT]]
    // auto handle_logout_user_request(std::string_view req_payload, std::span<char> resp_payload)
    //     -> kosio::async::Task<vrpc::InvokeResult>;
    //
    // [[REMEMBER_CO_AWAIT]]
    // auto handle_update_user_name_request(std::string_view req_payload, std::span<char> resp_payload)
    //     -> kosio::async::Task<vrpc::InvokeResult>;
    //
    // [[REMEMBER_CO_AWAIT]]
    // auto handle_update_user_avatar_request(std::string_view req_payload, std::span<char> resp_payload)
    //     -> kosio::async::Task<vrpc::InvokeResult>;
    //
    // [[REMEMBER_CO_AWAIT]]
    // auto handle_update_user_password_request(std::string_view req_payload, std::span<char> resp_payload)
    //     -> kosio::async::Task<vrpc::InvokeResult>;
    //
    // [[REMEMBER_CO_AWAIT]]
    // auto handle_update_user_role_request(std::string_view req_payload, std::span<char> resp_payload)
    //     -> kosio::async::Task<vrpc::InvokeResult>;
    //
    // [[REMEMBER_CO_AWAIT]]
    // auto handle_update_user_status_request(std::string_view req_payload, std::span<char> resp_payload)
    //     -> kosio::async::Task<vrpc::InvokeResult>;
    //
    // [[REMEMBER_CO_AWAIT]]
    // auto handle_update_user_quota_request(std::string_view req_payload, std::span<char> resp_payload)
    //     -> kosio::async::Task<vrpc::InvokeResult>;

private:
    kosio::sync::Mutex mutex_;
    sqlite3*           db_{nullptr};
    uint16_t           port_;
    std::string        ip_;
};
} // namespace vosfs::auth