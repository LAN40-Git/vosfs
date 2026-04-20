#include "vosfs-auth/client.hpp"
using namespace vosfs::auth;

class AuthClient : public BaseClient<AuthClient> {
    friend class BaseClient<AuthClient>;
public:
    explicit AuthClient(std::string_view server_ip, uint16_t server_port)
        : BaseClient<AuthClient>(server_ip, server_port) {}

private:
    [[REMEMBER_CO_AWAIT]]
    auto handle_register_user_response(vrpc::StatusCode code, std::string_view resp_payload) -> kosio::async::Task<void> {
        switch (code) {
            case vrpc::StatusCode::kOk: {
                LOG_INFO("注册成功");
                break;
            }
            case vrpc::StatusCode::kInvalidArgument: {
                LOG_INFO("密码错误");
                break;
            }
            case vrpc::StatusCode::kAlreadyExists: {
                LOG_INFO("用户名已存在");
                break;
            }
            default: {
                LOG_INFO("未知错误：{}", static_cast<uint8_t>(code));
                break;
            }
        }
        co_return;
    }

    [[REMEMBER_CO_AWAIT]]
    auto handle_delete_user_response(vrpc::StatusCode code, std::string_view resp_payload) -> kosio::async::Task<void> {
        switch (code) {
            case vrpc::StatusCode::kOk: {
                LOG_INFO("删除成功");
                break;
            }
            case vrpc::StatusCode::kPermissionDenied: {
                LOG_INFO("删除失败");
                break;
            }
            default: {
                LOG_INFO("未知错误：{}", static_cast<uint8_t>(code));
                break;
            }
        }
        co_return;
    }

    [[REMEMBER_CO_AWAIT]]
    auto handle_login_user_by_name_response(vrpc::StatusCode code, std::string_view resp_payload) -> kosio::async::Task<void> {
        LoginUserByNameResponse response;
        if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
            LOG_INFO("登录失败");
            co_return;
        }

        switch (code) {
            case vrpc::StatusCode::kOk: {
                session_.token = response.token();
                auto decoded = jwt::decode(session_.token);
                session_.uid = std::stoll(decoded.get_payload_claim("uid").as_string());
                session_.role = static_cast<User_Role>(std::stoi(decoded.get_payload_claim("role").as_string()));
                session_.quota = std::stoull(decoded.get_payload_claim("quota").as_string());
                session_.user_name = response.user_name();
                session_.avatar = response.avatar();
                session_.email = response.email();
                session_.phone = response.phone();
                session_.create_time = response.create_time();
                LOG_INFO("登录成功，用户{}-{}，创建时间-{}", session_.uid, session_.user_name, session_.create_time);
                break;
            }
            case vrpc::StatusCode::kInvalidArgument: {
                LOG_INFO("账号或密码错误");
                break;
            }
            case vrpc::StatusCode::kPermissionDenied: {
                LOG_INFO("用户被禁用或注销");
                break;
            }
            default: {
                LOG_INFO("未知错误：{}", static_cast<uint8_t>(code));
                break;
            }
        }
    }
};

auto main_loop() -> kosio::async::Task<void> {
    auto auth_client = AuthClient{"127.0.0.1", 8080};
    int n{1};
    while (n > 0) {
        --n;
        co_await auth_client.send_register_user_request("xiaoming", "123456", User_Role_kUser);
        co_await kosio::time::sleep(1000);
        co_await auth_client.send_login_user_by_name_request("xiaohong", "123456", User_Role_kUser);
        co_await kosio::time::sleep(1000);
        co_await auth_client.send_login_user_by_name_request("xiaoming", "111111", User_Role_kUser);
        co_await kosio::time::sleep(1000);
        co_await auth_client.send_login_user_by_name_request("xiaoming", "123456", User_Role_kAdmin);
        co_await kosio::time::sleep(1000);
        co_await auth_client.send_login_user_by_name_request("xiaoming", "123456", User_Role_kUser);
        co_await kosio::time::sleep(1000);
        co_await auth_client.send_login_user_by_name_request("xiaoming", "123456", User_Role_kUser);
        co_await kosio::time::sleep(1000);
        co_await auth_client.send_delete_user_request("123456");
        co_await kosio::time::sleep(1000);
        co_await auth_client.send_login_user_by_name_request("xiaoming", "123456", User_Role_kUser);
    }
}

auto main() -> int {
    kosio::runtime::CurrentThreadBuilder::default_create().block_on(main_loop());
}