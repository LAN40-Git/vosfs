#pragma once
#include <jwt-cpp/jwt.h>
#include <openssl/evp.h>
#include <vrpc/net/tcp/tcp_client.hpp>
#include "vosfs/common/error.hpp"
#include "status.hpp"
#include "vosfs/auth/detail/user_session.hpp"

namespace vosfs::auth {
template <typename Client>
class BaseClient {
public:
    explicit BaseClient(std::string_view server_ip, uint16_t server_port)
        : rpc_client_(server_ip, server_port) {}

public:
    [[REMEMBER_CO_AWAIT]]
    auto send_register_user_request(const std::string& user_name, const std::string& password, int role, std::string admin_secret = "") -> kosio::async::Task<void> {
        RegisterUserRequest request;
        request.set_user_name(std::move(user_name));
        request.set_password(sha256(password));
        request.set_role(static_cast<User_Role>(role));
        request.set_admin_secret(std::move(admin_secret));
        co_await rpc_client_.call_method<RegisterUserRequest, RegisterUserResponse>(
            "user", "register", request,
            [this](const vrpc::Status& status, const RegisterUserResponse& response) -> kosio::async::Task<void> {
                static_cast<Client*>(this)->handle_register_user_response(status, response);
                co_return;
            });
    }

    [[REMEMBER_CO_AWAIT]]
    auto send_delete_user_request(const std::string& password) -> kosio::async::Task<void> {
        DeleteUserRequest request;
        request.set_token(session_.token);
        request.set_password(sha256(password));
        co_await rpc_client_.call_method<DeleteUserRequest, DeleteUserResponse>(
            "user", "delete", request,
            [this](const vrpc::Status& status, const DeleteUserResponse& response) -> kosio::async::Task<void> {
                static_cast<Client*>(this)->handle_delete_user_response(status, response);
                co_return;
            });
    }

    [[REMEMBER_CO_AWAIT]]
    auto send_login_user_by_name_request(const std::string& user_name, const std::string& password, int role) -> kosio::async::Task<void> {
        LoginUserByNameRequest request;
        request.set_user_name(user_name);
        request.set_password(sha256(password));
        request.set_role(static_cast<User_Role>(role));
        co_await rpc_client_.call_method<LoginUserByNameRequest, LoginUserByNameResponse>(
            "user", "loginbyname", request,
            [this](const vrpc::Status& status, const LoginUserByNameResponse& response) -> kosio::async::Task<void> {
                static_cast<Client*>(this)->handle_login_user_by_name_response(status, response);
                co_return;
            });
    }

private:
    // [[REMEMBER_CO_AWAIT]]
    // auto handle_register_user_response(vrpc::StatusCode code, std::string_view resp_payload) -> kosio::async::Task<void> {
    //     co_await static_cast<Client*>(this)->handle_response(code);
    // }
    //
    // [[REMEMBER_CO_AWAIT]]
    // auto handle_delete_user_response(vrpc::StatusCode code, std::string_view resp_payload) -> kosio::async::Task<void> {
    //     co_await static_cast<Client*>(this)->handle_response(code);
    // }
    //
    // [[REMEMBER_CO_AWAIT]]
    // auto handle_login_user_by_name_response(vrpc::StatusCode code, std::string_view resp_payload) -> kosio::async::Task<void> {
    //     co_await static_cast<Client*>(this)->handle_response(code);
    // }

private:
    [[nodiscard]]
    static auto sha256(const std::string& input) -> std::string {
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) {
            return "";
        }

        EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
        EVP_DigestUpdate(ctx, input.c_str(), input.size());

        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int length = 0;
        EVP_DigestFinal_ex(ctx, hash, &length);

        EVP_MD_CTX_free(ctx);

        std::stringstream ss;
        for (unsigned int i = 0; i < length; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }

        return ss.str();
    }

protected:
    detail::UserSession session_{};

private:
    vrpc::TcpClient rpc_client_;
};
} // namespace vosfs::auth