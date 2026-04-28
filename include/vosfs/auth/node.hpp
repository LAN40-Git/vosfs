#pragma once
#include <sqlite3.h>
#include <vrpc/net/tcp/tcp_server.hpp>
#include "authpb/auth.pb.h"
#include "vosfs/common/error.hpp"

namespace vosfs::auth {
class AuthNode {
private:
    explicit AuthNode(sqlite3* db, std::string_view host = "0.0.0.0", uint16_t port = 9000)
        : db_(db)
        , host_(host)
        , port_(port) {
        assert(db_);
    }

public:
    ~AuthNode() {
        if (db_) {
            sqlite3_close(db_);
        }
    }

public:
    [[nodiscard]]
    static auto create(
        std::string_view db_path,
        std::string_view host = "0.0.0.0",
        uint16_t port = 9000) -> Result<std::unique_ptr<AuthNode>>;

public:
    auto wait() -> kosio::async::Task<void>;

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
    std::string        host_;
    uint16_t           port_;
};
} // namespace vosfs::auth