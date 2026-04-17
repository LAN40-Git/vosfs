#include <chrono>
#include <kosio/signal/signal.hpp>
#include "auth.pb.h"
#include "vosfs/rpc.hpp"
#include "vosfs/common/util/message_factory.hpp"

using namespace vosfs;
using namespace vosfs::rpc;
using namespace vosfs::auth;

uint64_t g_uid;

auto send_put_user_request(const std::unique_ptr<RpcConsumer>& consumer) -> kosio::async::Task<void>;
auto handle_put_user_response(std::string_view resp_payload) -> kosio::async::Task<void>;
auto send_get_user_request(const std::unique_ptr<RpcConsumer>& consumer) -> kosio::async::Task<void>;
auto handle_get_user_response(std::string_view resp_payload) -> kosio::async::Task<void>;
auto send_update_user_name_request(const std::unique_ptr<RpcConsumer>& consumer) -> kosio::async::Task<void>;
auto handle_update_user_name_response(std::string_view resp_payload) -> kosio::async::Task<void>;
auto send_update_user_password_request(const std::unique_ptr<RpcConsumer>& consumer) -> kosio::async::Task<void>;
auto handle_update_user_password_response(std::string_view resp_payload) -> kosio::async::Task<void>;
auto send_update_user_role_request(const std::unique_ptr<RpcConsumer>& consumer) -> kosio::async::Task<void>;
auto handle_update_user_role_response(std::string_view resp_payload) -> kosio::async::Task<void>;
auto send_delete_user_request(const std::unique_ptr<RpcConsumer>& consumer) -> kosio::async::Task<void>;
auto handle_delete_user_response(std::string_view resp_payload) -> kosio::async::Task<void>;

auto send_put_user_request(const std::unique_ptr<RpcConsumer>& consumer) -> kosio::async::Task<void> {
    std::string name, password;
    int role;
    std::cout << "用户名:";
    std::cin >> name;
    std::cout << "密码:";
    std::cin >> password;
    std::cout << "角色(0-Admin, 1-User):";
    std::cin >> role;
    auto request = util::MessageFactory::make_put_user_request(std::move(name), std::move(password), role);

    co_await consumer->send_request(
        ServiceType::kAuth,
        MethodType::kAuthPutUser,
        request,
        handle_put_user_response);
}

auto handle_put_user_response(std::string_view resp_payload) -> kosio::async::Task<void> {
    PutUserResponse response;
    if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
        LOG_ERROR("failed to parse rpc response");
        co_return;
    }

    [[maybe_unused]] auto success = response.success();
    auto& msg = response.msg();
    LOG_INFO("{}", msg);
}

auto send_get_user_request(const std::unique_ptr<RpcConsumer>& consumer) -> kosio::async::Task<void> {
    std::string name, password;
    int role;
    std::cout << "用户名:";
    std::cin >> name;
    std::cout << "密码:";
    std::cin >> password;
    std::cout << "角色:";
    std::cin >> role;
    auto request = util::MessageFactory::make_get_user_request(std::move(name), std::move(password), role);

    co_await consumer->send_request(
        ServiceType::kAuth,
        MethodType::kAuthGetUser,
        request,
        handle_get_user_response);
}

auto handle_get_user_response(std::string_view resp_payload) -> kosio::async::Task<void> {
    GetUserResponse response;
    if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
        LOG_ERROR("failed to parse rpc response");
        co_return;
    }

    auto success = response.success();
    auto& msg = response.msg();
    auto uid = response.uid();
    auto& name = response.name();
    auto role = response.role();
    auto role_payload = role == kAdmin ? "admin" : "user";
    auto create_time = response.create_time();
    LOG_INFO("{}", msg);
    if (success) {
        g_uid = uid;
        LOG_INFO("Get user: uid-{}, name-{}, role-{}, create_time-{}", uid, name, role_payload, kosio::util::format_time(create_time));
    }
}

auto send_update_user_name_request(const std::unique_ptr<RpcConsumer>& consumer) -> kosio::async::Task<void> {
    std::string name;
    std::cout << "新用户名:";
    std::cin >> name;
    auto request = util::MessageFactory::make_update_user_name_request(g_uid, std::move(name));

    co_await consumer->send_request(
        ServiceType::kAuth,
        MethodType::kAuthUpdateUserName,
        request,
        handle_update_user_name_response);
}

auto handle_update_user_name_response(std::string_view resp_payload) -> kosio::async::Task<void> {
    UpdateUserNameResponse response;
    if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
        LOG_ERROR("failed to parse rpc response");
        co_return;
    }

    [[maybe_unused]] auto success = response.success();
    auto& msg = response.msg();
    LOG_INFO("{}", msg);
}

auto send_update_user_password_request(const std::unique_ptr<RpcConsumer>& consumer) -> kosio::async::Task<void> {
    std::string password;
    std::cout << "新密码:";
    std::cin >> password;
    auto request = util::MessageFactory::make_update_user_password_request(g_uid, std::move(password));
    co_await consumer->send_request(
        ServiceType::kAuth,
        MethodType::kAuthUpdateUserPassword,
        request,
        handle_update_user_password_response);
}

auto handle_update_user_password_response(std::string_view resp_payload) -> kosio::async::Task<void> {
    UpdateUserPasswordResponse response;
    if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
        LOG_ERROR("failed to parse rpc response");
        co_return;
    }

    [[maybe_unused]] auto success = response.success();
    auto& msg = response.msg();
    LOG_INFO("{}", msg);
}

auto send_update_user_role_request(const std::unique_ptr<RpcConsumer>& consumer) -> kosio::async::Task<void> {
    int role;
    std::cout << "新角色:";
    std::cin >> role;
    auto request = util::MessageFactory::make_update_user_role_request(g_uid, role);
    co_await consumer->send_request(
        ServiceType::kAuth,
        MethodType::kAuthUpdateUserRole,
        request,
        handle_update_user_role_response);
}

auto handle_update_user_role_response(std::string_view resp_payload) -> kosio::async::Task<void> {
    UpdateUserRoleResponse response;
    if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
        LOG_ERROR("failed to parse rpc response");
        co_return;
    }

    [[maybe_unused]] auto success = response.success();
    auto& msg = response.msg();
    LOG_INFO("{}", msg);
}

auto send_delete_user_request(const std::unique_ptr<RpcConsumer>& consumer) -> kosio::async::Task<void> {
    std::string password;
    std::cout << "密码:";
    std::cin >> password;
    auto request = util::MessageFactory::make_delete_user_request(g_uid, std::move(password));
    co_await consumer->send_request(
        ServiceType::kAuth,
        MethodType::kAuthDeleteUser,
        request,
        handle_delete_user_response);
}

auto handle_delete_user_response(std::string_view resp_payload) -> kosio::async::Task<void> {
    DeleteUserResponse response;
    if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
        LOG_ERROR("failed to parse rpc response");
        co_return;
    }

    [[maybe_unused]] auto success = response.success();
    auto& msg = response.msg();
    LOG_INFO("{}", msg);
}

auto process(std::unique_ptr<RpcConsumer> consumer) -> kosio::async::Task<void> {
    co_await send_put_user_request(consumer);
    co_await kosio::time::sleep(1000);
    co_await send_get_user_request(consumer);
    co_await kosio::time::sleep(1000);
    co_await send_update_user_name_request(consumer);
    co_await kosio::time::sleep(1000);
    co_await send_update_user_password_request(consumer);
    co_await kosio::time::sleep(1000);
    co_await send_update_user_role_request(consumer);
    co_await kosio::time::sleep(1000);
    co_await send_delete_user_request(consumer);
    co_await kosio::signal::ctrl_c();
    co_await consumer->shutdown();
}

auto main_loop() -> kosio::async::Task<void> {
    auto has_consumer = co_await RpcConsumer::create("172.18.207.176", 8080);
    if (!has_consumer) {
        LOG_ERROR("failed to create consumer: {}", has_consumer.error());
        co_return;
    }

    co_await process(std::move(has_consumer.value()));
}

auto main() -> int {
    SET_LOG_LEVEL(kosio::log::LogLevel::Verbose);
    kosio::runtime::MultiThreadBuilder::default_create().block_on(main_loop());
}
