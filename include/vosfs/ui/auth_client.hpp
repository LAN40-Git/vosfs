#pragma once
#include <QObject>
#undef emit
#include <QDebug>
#include <jwt-cpp/jwt.h>
#include <vrpc/net/tcp/tcp_client.hpp>
#include "vosfs/common/error.hpp"
#include "vosfs/common/util/sha256.hpp"
#include "vosfs/auth/status.hpp"
#include "vosfs/auth/user_session.hpp"

namespace vosfs::ui {
using auth::detail::UserSession;
class AuthClient : public QObject {
    Q_OBJECT
public:
    explicit AuthClient(std::string_view host, uint16_t port, QObject *parent = nullptr);

public slots:
    void register_user(const QString& user_name, const QString& password, int role, const QString& admin_secret = "");
    void delete_user(const QString& password);
    void login_user_by_name(const QString& user_name, const QString& password, int role);

public:
    [[REMEMBER_CO_AWAIT]]
    auto send_register_user_request(
        std::string user_name,
        std::string password,
        int role,
        std::string admin_secret = "") -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto send_delete_user_request(
        std::string password) -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto send_login_user_by_name_request(
        std::string user_name,
        std::string password,
        int role) -> kosio::async::Task<void>;

private:
    void handle_register_user_response(const vrpc::Status& status, const auth::RegisterUserResponse& response);
    void handle_delete_user_response(const vrpc::Status& status, const auth::DeleteUserResponse& response);
    void handle_login_user_by_name_response(const vrpc::Status& status, const auth::LoginUserByNameResponse& response);

private:
    UserSession     session_{};
    vrpc::TcpClient rpc_client_;
};
} // namespace vosfs::ui