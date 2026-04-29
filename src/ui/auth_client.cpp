#include "vosfs/ui/auth_client.hpp"
#include "vosfs/common/util/sha256.hpp"
#include "vosfs/auth/status.hpp"

vosfs::ui::AuthClient::AuthClient(std::string_view host, uint16_t port, QObject* parent)
    : QObject(parent)
    , rpc_client_(host, port) {}

void vosfs::ui::AuthClient::run() {
    is_shutdown_.store(false, std::memory_order_release);
    kosio::runtime::CurrentThreadBuilder::default_create().block_on(process_tasks());
    latch_.count_down();
}

void vosfs::ui::AuthClient::shutdown() {
    if (is_shutdown_.load(std::memory_order_acquire)) {
        return;
    }
    tasks_.emplace(rpc_client_.shutdown());
    is_shutdown_.store(true, std::memory_order_release);
    latch_.wait();
}

auto vosfs::ui::AuthClient::process_tasks() -> Task<void> {
    static constexpr std::size_t MAX_POP_SIZE = 16;
    while (!is_shutdown_.load(std::memory_order_relaxed) || !tasks_.empty()) {
        int n = 0;
        Task<void> task;
        while (n < MAX_POP_SIZE) {
            if (!tasks_.try_pop(task)) {
                break;
            } else {
                co_await task;
                ++n;
            }
        }
        co_await kosio::time::sleep(10);
    }
}

void vosfs::ui::AuthClient::register_user(
    const QString& user_name,
    const QString& password,
    int role,
    const QString& admin_secret) {
    tasks_.emplace(send_register_user_request(
        user_name.toStdString(),
        password.toStdString(),
        role,
        admin_secret.toStdString()));
}

void vosfs::ui::AuthClient::delete_user(const QString& password) {
    tasks_.emplace(send_delete_user_request(password.toStdString()));
}

void vosfs::ui::AuthClient::login_user_by_name(const QString& user_name, const QString& password, int role) {
    tasks_.emplace(send_login_user_by_name_request(
        user_name.toStdString(),
        password.toStdString(),
        role));
}

auto vosfs::ui::AuthClient::send_register_user_request(
    std::string user_name,
    std::string password,
    int role,
    std::string admin_secret) -> Task<void> {
    auth::RegisterUserRequest request;
    request.set_user_name(std::move(user_name));
    request.set_password(util::sha256(password));
    request.set_role(static_cast<auth::User_Role>(role));
    request.set_admin_secret(std::move(admin_secret));
    co_await rpc_client_.call_method<auth::RegisterUserRequest, auth::RegisterUserResponse>(
        "user", "register", request,
        [this](const vrpc::Status& status, const auth::RegisterUserResponse& response) -> Task<void> {
            this->handle_register_user_response(status, response);
            co_return;
        });
}

auto vosfs::ui::AuthClient::send_delete_user_request(std::string password) -> Task<void> {
    auth::DeleteUserRequest request;
    request.set_token(session_.token);
    request.set_password(util::sha256(password));
    co_await rpc_client_.call_method<auth::DeleteUserRequest, auth::DeleteUserResponse>(
        "user", "delete", request,
        [this](const vrpc::Status& status, const auth::DeleteUserResponse& response) -> Task<void> {
            this->handle_delete_user_response(status, response);
            co_return;
        });
}

auto vosfs::ui::AuthClient::send_login_user_by_name_request(
    std::string user_name,
    std::string password,
    int role) -> Task<void> {
    auth::LoginUserByNameRequest request;
    request.set_user_name(std::move(user_name));
    request.set_password(util::sha256(password));
    request.set_role(static_cast<auth::User_Role>(role));
    co_await rpc_client_.call_method<auth::LoginUserByNameRequest, auth::LoginUserByNameResponse>(
        "user", "loginbyname", request,
        [this](const vrpc::Status& status, const auth::LoginUserByNameResponse& response) -> Task<void> {
            this->handle_login_user_by_name_response(status, response);
            co_return;
        });
}

void vosfs::ui::AuthClient::handle_register_user_response(
    const vrpc::Status& status,
    const auth::RegisterUserResponse& response) {
    if (!status.ok()) {
        LOG_ERROR("{}", status.message());
        return;
    }
    LOG_INFO("{}", response.message());
}

void vosfs::ui::AuthClient::handle_delete_user_response(
    const vrpc::Status& status,
    const auth::DeleteUserResponse& response) {
    if (!status.ok()) {
        LOG_ERROR("{}", status.message());
        return;
    }
    LOG_INFO("{}", response.message());
}

void vosfs::ui::AuthClient::handle_login_user_by_name_response(
    const vrpc::Status& status,
    const auth::LoginUserByNameResponse& response) {
    if (!status.ok()) {
        LOG_ERROR("{}", status.message());
        return;
    }
    LOG_INFO("{}", response.message());
}
