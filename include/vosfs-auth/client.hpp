#pragma once
#include <jwt-cpp/jwt.h>
#include <vrpc/client.hpp>
#include "vosfs/common/error.hpp"
#include "vosfs-auth/detail/rpc.hpp"
#include "vosfs-auth/detail/user_session.hpp"

namespace vosfs::auth {
template <typename Client>
class BaseClient {
public:
    explicit BaseClient(std::string_view server_ip, uint16_t server_port)
        : rpc_client_(server_ip, server_port) {}

public:
    [[REMEMBER_CO_AWAIT]]
    auto send_register_user_request(std::string user_name, std::string password, int role, std::string admin_secret = "") -> kosio::async::Task<void> {
        RegisterUserRequest request;
        request.set_user_name(std::move(user_name));
        request.set_password(std::move(password));
        request.set_role(static_cast<User_Role>(role));
        request.set_admin_secret(std::move(admin_secret));
        co_await rpc_client_.call(
            detail::ServiceType::kUser,
            detail::InvokeType::kRegisterUser,
            request,
            [this](vrpc::StatusCode code, std::string_view resp_payload) -> kosio::async::Task<void> {
                co_await this->handle_register_user_response(code, resp_payload);
            });
    }

    [[REMEMBER_CO_AWAIT]]
    auto send_delete_user_request(std::string password) -> kosio::async::Task<void> {

    }

    [[REMEMBER_CO_AWAIT]]
    auto send_login_user_by_name_request(std::string user_name, std::string password) -> kosio::async::Task<void>;

public:
    [[REMEMBER_CO_AWAIT]]
    auto handle_register_user_response(vrpc::StatusCode code, std::string_view resp_payload) -> kosio::async::Task<void> {
        co_await static_cast<Client*>(this)->handle_response(code, resp_payload);
    }

    [[REMEMBER_CO_AWAIT]]
    auto handle_delete_user_response(vrpc::StatusCode code, std::string_view resp_payload) -> kosio::async::Task<void> {
        co_await static_cast<Client*>(this)->handle_response(code);
    }

    [[REMEMBER_CO_AWAIT]]
    auto handle_login_user_by_name_response(vrpc::StatusCode code, std::string_view resp_payload) -> kosio::async::Task<void> {
        LoginUserByNameResponse response;
        if (response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
            co_await mutex_.lock();
            session_.token = response.token();
            auto decoded = jwt::decode(session_.token);
            session_.uid = std::stoll(decoded.get_payload_claim("uid"));
            session_.role = std::stoi(decoded.get_payload_claim("role"));
            session_.quota = std::stoull(decoded.get_payload_claim("quota"));
            session_.user_name = response.user_name();
            session_.avatar = response.avatar();
            session_.email = response.email();
            session_.phone = response.phone();
            co_await mutex_.unlock();
        }
        co_await static_cast<Client*>(this)->handle_response(code);
    }

private:
    kosio::sync::SharedMutex mutex_;
    detail::UserSession      session_;
    vrpc::Client             rpc_client_;
};
} // namespace vosfs::auth