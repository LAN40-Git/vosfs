#pragma once
#include <QDebug>
#include "vosfs/ui/signal_brige.hpp"
#undef emit
#include <tbb/concurrent_queue.h>
#include <jwt-cpp/jwt.h>
#include <vrpc/net/tcp/tcp_client.hpp>
#include "vosfs/common/error.hpp"
#include "vosfs/auth/user_session.hpp"

namespace vosfs::ui {
using auth::detail::UserSession;
using kosio::async::Task;
class VosfsClient : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString uid READ uid)
    Q_PROPERTY(QString user_name READ user_name)
    Q_PROPERTY(QString avatar READ avatar)
    Q_PROPERTY(int role READ role)
    Q_PROPERTY(QString quota READ quota)
    Q_PROPERTY(QString create_time READ create_time)
public:
    explicit VosfsClient(std::string_view host, uint16_t port, SignalBrige& signal_brige, QObject *parent = nullptr);

public:
    void run();
    void shutdown();
    auto process_tasks() -> Task<void>;

public:
    [[nodiscard]]
    auto uid() const -> QString { return QString::number(session_.uid); }
    [[nodiscard]]
    auto user_name() const -> QString { return QString::fromStdString(session_.user_name); }
    [[nodiscard]]
    auto avatar() const -> QString { return QString::fromStdString(session_.avatar); }
    [[nodiscard]]
    auto role() const -> int { return session_.role; }
    [[nodiscard]]
    auto quota() const -> QString { return QString::number(session_.quota); }
    [[nodiscard]]
    auto create_time() const -> QString { return QString::fromStdString(session_.create_time); }

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
        std::string admin_secret = "") -> Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto send_delete_user_request(
        std::string password) -> Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto send_login_user_by_name_request(
        std::string user_name,
        std::string password,
        int role) -> Task<void>;

private:
    void handle_register_user_response(const vrpc::Status& status, const auth::RegisterUserResponse& response);
    void handle_delete_user_response(const vrpc::Status& status, const auth::DeleteUserResponse& response);
    void handle_login_user_by_name_response(const vrpc::Status& status, const auth::LoginUserByNameResponse& response);

private:
    std::atomic<bool>                 is_shutdown_{true};
    std::latch                        latch_{1};
    UserSession                       session_{};
    vrpc::TcpClient                   rpc_client_;
    tbb::concurrent_queue<Task<void>> tasks_;
    SignalBrige&                      signal_brige_;
};
} // namespace vosfs::ui