#pragma once
#include <QDebug>
#include <QVariant>
#include "vosfs/ui/signal_brige.hpp"
#undef emit
#include <tbb/concurrent_queue.h>
#include <jwt-cpp/jwt.h>
#include <vrpc/net/tcp/tcp_client.hpp>
#include "raftpb/raft.pb.h"
#include "vosfs/ui/config.hpp"
#include "vosfs/ui/transport_task.hpp"
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

    using RPCClient = std::unique_ptr<vrpc::TcpClient>;

public:
    explicit VosfsClient(
        RPCClient auth_client,
        std::unordered_map<uint64_t, RPCClient> raft_clients,
        std::unordered_map<uint64_t, RPCClient> data_clients,
        SignalBrige& signal_brige,
        QObject *parent = nullptr);

public:
    static auto create(const std::string& config_path, SignalBrige& signal_brige) -> Result<std::unique_ptr<VosfsClient>>;

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
    void list_dir(const QString& path);
    void make_dir(const QString& parent_path, const QString& name);
    void prepare_upload_file(const QString& local_path, const QString& current_dir);

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

    [[REMEMBER_CO_AWAIT]]
    auto send_list_dir_request(std::string path) -> Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto send_make_dir_request(
        std::string parent_path,
        std::string name) -> Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto send_prepare_upload_file_request(
        std::string local_path, std::string current_dir) -> Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto send_upload_block_request(
        const std::string& local_path,
        uint64_t block_id,
        uint64_t ino,
        uint64_t offset,
        uint64_t size,
        uint64_t data_node_id) -> Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto send_upload_file_request(
        uint64_t ino) -> Task<void>;

private:
    void handle_register_user_response(const vrpc::Status& status, const auth::RegisterUserResponse& response);
    void handle_delete_user_response(const vrpc::Status& status, const auth::DeleteUserResponse& response);
    void handle_login_user_by_name_response(const vrpc::Status& status, const auth::LoginUserByNameResponse& response);
    auto handle_list_dir_response(const vrpc::Status& status, const raft::ListDirResponse& response) -> Task<void>;
    auto handle_make_dir_response(const vrpc::Status& status, const raft::MakeDirResponse& response) -> Task<void>;
    auto handle_prepare_upload_file_response(const vrpc::Status& status, const raft::PrepareUploadFileResponse& response) -> Task<void>;
    auto handle_upload_block_response(const vrpc::Status& status, const raft::UploadBlockResponse& response) -> Task<void>;

private:
    void load_transport_tasks_json();
    auto do_upload_task(const detail::TransportTask& task) -> Task<void>;
    auto do_download_task(const detail::TransportTask& task);

private:
    std::atomic<bool>                       is_shutdown_{true};
    std::latch                              latch_{1};
    kosio::sync::Mutex                      mutex_;
    UserSession                             session_{};
    RPCClient                               auth_client_;
    uint64_t                                leader_id_{0};
    std::unordered_map<uint64_t, RPCClient> raft_clients_;
    std::unordered_map<uint64_t, RPCClient> data_clients_;
    tbb::concurrent_queue<Task<void>>       tasks_;
    nlohmann::json                          json_;
    std::unordered_map<uint64_t, detail::TransportTask> upload_tasks_;
    std::unordered_map<uint64_t, detail::TransportTask> download_tasks_;
    SignalBrige&                            signal_brige_;
};
} // namespace vosfs::ui