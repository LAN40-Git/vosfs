#include "vosfs/ui/auth_client.hpp"

vosfs::ui::AuthClient::AuthClient(std::string_view host, uint16_t port, QObject* parent)
    : QObject(parent)
    , rpc_client_(host, port) {}

void vosfs::ui::AuthClient::register_user(
    const QString& user_name,
    const QString& password,
    int role,
    const QString& admin_secret) {
    QMetaObject::invokeMethod(this, [this, user_name, password, role, admin_secret]() {
        kosio::spawn(send_register_user_request(
            user_name.toStdString(),
            password.toStdString(),
            role,
            admin_secret.toStdString()
        ));
    }, Qt::QueuedConnection);
    // kosio::spawn(send_register_user_request(
    //     user_name.toStdString(),
    //     password.toStdString(),
    //     role,
    //     admin_secret.toStdString()));
}

void vosfs::ui::AuthClient::delete_user(const QString& password) {
    kosio::spawn(send_delete_user_request(password.toStdString()));
}

void vosfs::ui::AuthClient::login_user_by_name(const QString& user_name, const QString& password, int role) {
    kosio::spawn(send_login_user_by_name_request(
        user_name.toStdString(),
        password.toStdString(),
        role));
}

auto vosfs::ui::AuthClient::send_register_user_request(
    std::string user_name,
    std::string password,
    int role,
    std::string admin_secret) -> kosio::async::Task<void> {
    LOG_INFO("SHIT");
    auth::RegisterUserRequest request;
    request.set_user_name(std::move(user_name));
    request.set_password(util::sha256(password));
    request.set_role(static_cast<auth::User_Role>(role));
    request.set_admin_secret(std::move(admin_secret));
    co_await rpc_client_.call_method<auth::RegisterUserRequest, auth::RegisterUserResponse>(
        "user", "register", request,
        [this](const vrpc::Status& status, const auth::RegisterUserResponse& response) -> kosio::async::Task<void> {
            this->handle_register_user_response(status, response);
            co_return;
        });
}

auto vosfs::ui::AuthClient::send_delete_user_request(std::string password) -> kosio::async::Task<void> {
    auth::DeleteUserRequest request;
    request.set_token(session_.token);
    request.set_password(util::sha256(password));
    co_await rpc_client_.call_method<auth::DeleteUserRequest, auth::DeleteUserResponse>(
        "user", "delete", request,
        [this](const vrpc::Status& status, const auth::DeleteUserResponse& response) -> kosio::async::Task<void> {
            this->handle_delete_user_response(status, response);
            co_return;
        });
}

auto vosfs::ui::AuthClient::send_login_user_by_name_request(
    std::string user_name,
    std::string password,
    int role) -> kosio::async::Task<void> {
    auth::LoginUserByNameRequest request;
    request.set_user_name(std::move(user_name));
    request.set_password(util::sha256(password));
    request.set_role(static_cast<auth::User_Role>(role));
    co_await rpc_client_.call_method<auth::LoginUserByNameRequest, auth::LoginUserByNameResponse>(
        "user", "loginbyname", request,
        [this](const vrpc::Status& status, const auth::LoginUserByNameResponse& response) -> kosio::async::Task<void> {
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
