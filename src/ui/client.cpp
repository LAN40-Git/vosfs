#include <QVariantMap>
#include "vosfs/ui/client.hpp"
#include "vosfs/common/util/sha256.hpp"
#include "vosfs/common/status.hpp"

vosfs::ui::VosfsClient::VosfsClient(std::string_view host, uint16_t port, SignalBrige& signal_brige, QObject* parent)
    : QObject(parent)
    , auth_client_(host, port)
    , signal_brige_(signal_brige) {}

void vosfs::ui::VosfsClient::run() {
    is_shutdown_.store(false, std::memory_order_release);
    kosio::runtime::CurrentThreadBuilder::default_create().block_on(process_tasks());
    latch_.count_down();
}

void vosfs::ui::VosfsClient::shutdown() {
    if (is_shutdown_.load(std::memory_order_acquire)) {
        return;
    }
    tasks_.emplace(auth_client_.shutdown());
    is_shutdown_.store(true, std::memory_order_release);
    latch_.wait();
}

auto vosfs::ui::VosfsClient::process_tasks() -> Task<void> {
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

void vosfs::ui::VosfsClient::register_user(
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

void vosfs::ui::VosfsClient::delete_user(const QString& password) {
    tasks_.emplace(send_delete_user_request(password.toStdString()));
}

void vosfs::ui::VosfsClient::login_user_by_name(const QString& user_name, const QString& password, int role) {
    tasks_.emplace(send_login_user_by_name_request(
        user_name.toStdString(),
        password.toStdString(),
        role));
}

void vosfs::ui::VosfsClient::list_dir(const QString& parent_ino) {
    tasks_.emplace(send_list_dir_request(parent_ino.toULongLong()));
}

auto vosfs::ui::VosfsClient::send_register_user_request(
    std::string user_name,
    std::string password,
    int role,
    std::string admin_secret) -> Task<void> {
    auth::RegisterUserRequest request;
    request.set_user_name(std::move(user_name));
    request.set_password(util::sha256(password));
    request.set_role(static_cast<auth::User_Role>(role));
    request.set_admin_secret(std::move(admin_secret));
    co_await auth_client_.call_method<auth::RegisterUserRequest, auth::RegisterUserResponse>(
        "user", "register", request,
        [this](const vrpc::Status& status, const auth::RegisterUserResponse& response) -> Task<void> {
            this->handle_register_user_response(status, response);
            co_return;
        });
}

auto vosfs::ui::VosfsClient::send_delete_user_request(std::string password) -> Task<void> {
    auth::DeleteUserRequest request;
    request.set_token(session_.token);
    request.set_password(util::sha256(password));
    co_await auth_client_.call_method<auth::DeleteUserRequest, auth::DeleteUserResponse>(
        "user", "delete", request,
        [this](const vrpc::Status& status, const auth::DeleteUserResponse& response) -> Task<void> {
            this->handle_delete_user_response(status, response);
            co_return;
        });
}

auto vosfs::ui::VosfsClient::send_login_user_by_name_request(
    std::string user_name,
    std::string password,
    int role) -> Task<void> {
    auth::LoginUserByNameRequest request;
    request.set_user_name(std::move(user_name));
    request.set_password(util::sha256(password));
    request.set_role(static_cast<auth::User_Role>(role));
    co_await auth_client_.call_method<auth::LoginUserByNameRequest, auth::LoginUserByNameResponse>(
        "user", "loginbyname", request,
        [this](const vrpc::Status& status, const auth::LoginUserByNameResponse& response) -> Task<void> {
            this->handle_login_user_by_name_response(status, response);
            co_return;
        });
}

auto vosfs::ui::VosfsClient::send_list_dir_request(uint64_t parent_ino) -> Task<void> {
    raft::ListDirRequest request;
    request.set_token(session_.token);
    request.set_parent_ino(parent_ino);
    co_await auth_client_.call_method<raft::ListDirRequest, raft::ListDirResponse>(
        "fs", "ls", request,
        [this](const vrpc::Status& status, const raft::ListDirResponse& response) -> Task<void> {
            this->handle_list_dir_response(status, response);
            co_return;
        });
}

auto vosfs::ui::VosfsClient::send_mkdir_request(uint64_t parent_ino, std::string name) -> Task<void> {
    raft::MakeDirRequest request;
    request.set_token(session_.token);
    request.set_parent_ino(parent_ino);
    request.set_name(std::move(name));
    co_await auth_client_.call_method<raft::MakeDirRequest, raft::MakeDirResponse>(
        "fs", "mkdir", request,
        [this](const vrpc::Status& status, const raft::MakeDirResponse& response) -> Task<void> {
            this->handle_make_dir_response(status, response);
            co_return;
        });
}

void vosfs::ui::VosfsClient::handle_register_user_response(
    const vrpc::Status& status,
    const auth::RegisterUserResponse& response) {
    if (!status.ok()) {
        signal_brige_.registerFinished(false, QString::fromStdString(std::string{status.message()}));
        return;
    }
    auto res_status = Status{response.status_code()};
    signal_brige_.appendLog(QString::fromStdString(std::string{response.message()}));
    signal_brige_.registerFinished(res_status.ok(), QString::fromStdString(response.message()));
}

void vosfs::ui::VosfsClient::handle_delete_user_response(
    const vrpc::Status& status,
    const auth::DeleteUserResponse& response) {
    if (!status.ok()) {
        signal_brige_.deleteFinished(false, QString::fromStdString(std::string{status.message()}));
        return;
    }
    auto res_status = Status{response.status_code()};
    signal_brige_.appendLog(QString::fromStdString(std::string{response.message()}));
    signal_brige_.deleteFinished(res_status.ok(), QString::fromStdString(response.message()));
}

void vosfs::ui::VosfsClient::handle_login_user_by_name_response(
    const vrpc::Status& status,
    const auth::LoginUserByNameResponse& response) {
    if (!status.ok()) {
        signal_brige_.loginFinished(false, QString::fromStdString(std::string{status.message()}));
        return;
    }
    auto res_status = Status{response.status_code()};
    if (res_status.ok()) {
        session_.token = response.token();
        auto decode = jwt::decode(session_.token);
        session_.uid = stoll(decode.get_payload_claim("uid").as_string());
        session_.user_name = response.user_name();
        session_.avatar = response.avatar();
        session_.role = static_cast<auth::User_Role>(stoi(decode.get_payload_claim("role").as_string()));
        session_.quota = static_cast<uint64_t>(stoull(decode.get_payload_claim("quota").as_string()));
        session_.create_time = response.create_time();
    }
    signal_brige_.appendLog(QString::fromStdString(std::string{response.message()}));
    signal_brige_.loginFinished(res_status.ok(), QString::fromStdString(response.message()));
}

auto vosfs::ui::VosfsClient::handle_list_dir_response(
    const vrpc::Status& status,
    const raft::ListDirResponse& response) -> Task<void> {
    if (!status.ok()) {
        signal_brige_.appendLog(QString::fromStdString(std::string{status.message()}));
        co_return;
    }

    auto res_status = Status{response.status_code()};
    if (res_status.ok()) {
        
    }
}

void vosfs::ui::VosfsClient::handle_make_dir_response(
    const vrpc::Status& status,
    const raft::MakeDirResponse& response) {
    if (!status.ok()) {
        signal_brige_.appendLog(QString::fromStdString(std::string{status.message()}));
        return;
    }

    auto res_status = Status{response.status_code()};

}
