#include <QVariantMap>
#include <kosio/fs.hpp>
#include "vosfs/ui/client.hpp"
#include "vosfs/common/util/sha256.hpp"
#include "vosfs/common/status.hpp"

vosfs::ui::VosfsClient::VosfsClient(
    RPCClient auth_client,
    std::unordered_map<uint64_t, RPCClient> raft_clients,
    std::unordered_map<uint64_t, RPCClient> data_clients,
    SignalBrige& signal_brige,
    QObject* parent)
    : QObject(parent)
    , auth_client_(std::move(auth_client))
    , raft_clients_(std::move(raft_clients))
    , data_clients_(std::move(data_clients))
    , signal_brige_(signal_brige) {
    if (!raft_clients_.empty()) {
        leader_id_ = raft_clients_.begin()->first;
    }
}

auto vosfs::ui::VosfsClient::create(
    const std::string& config_path,
    SignalBrige& signal_brige) -> Result<std::unique_ptr<VosfsClient>> {
    auto has_config = detail::Config::from_json(config_path);
    if (!has_config) {
        return std::unexpected{has_config.error()};
    }
    auto config = std::move(has_config.value());

    auto auth_client = std::make_unique<vrpc::TcpClient>(config.auth_host, config.auth_port);
    std::unordered_map<uint64_t, RPCClient> raft_clients;
    for (auto& node : config.raft_nodes | std::views::values) {
        raft_clients.emplace(node.id, std::make_unique<vrpc::TcpClient>(node.host, node.port));
    }
    std::unordered_map<uint64_t, RPCClient> data_clients;
    for (auto& node : config.data_nodes | std::views::values) {
        data_clients.emplace(node.id, std::make_unique<vrpc::TcpClient>(node.host, node.port));
    }
    return std::make_unique<VosfsClient>(
        std::move(auth_client),
        std::move(raft_clients),
        std::move(data_clients),
        signal_brige);
}

void vosfs::ui::VosfsClient::run() {
    is_shutdown_.store(false, std::memory_order_release);
    kosio::runtime::CurrentThreadBuilder::default_create().block_on(process_tasks());
    latch_.count_down();
}

void vosfs::ui::VosfsClient::shutdown() {
    if (is_shutdown_.load(std::memory_order_acquire)) {
        return;
    }
    tasks_.emplace(auth_client_->shutdown());
    for (auto& raft_client : raft_clients_ | std::views::values) {
        tasks_.emplace(raft_client->shutdown());
    }
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

void vosfs::ui::VosfsClient::list_dir(const QString& path) {
    tasks_.emplace(send_list_dir_request(path.toStdString()));
}

void vosfs::ui::VosfsClient::make_dir(const QString& parent_path, const QString& name) {
    tasks_.emplace(send_make_dir_request(parent_path.toStdString(), name.toStdString()));
}

void vosfs::ui::VosfsClient::prepare_upload_file(const QString& local_path, const QString& current_dir) {
    tasks_.emplace(send_prepare_upload_file_request(local_path.toStdString(), current_dir.toStdString()));
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
    co_await auth_client_->call_method<auth::RegisterUserRequest, auth::RegisterUserResponse>(
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
    co_await auth_client_->call_method<auth::DeleteUserRequest, auth::DeleteUserResponse>(
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
    co_await auth_client_->call_method<auth::LoginUserByNameRequest, auth::LoginUserByNameResponse>(
        "user", "loginbyname", request,
        [this](const vrpc::Status& status, const auth::LoginUserByNameResponse& response) -> Task<void> {
            this->handle_login_user_by_name_response(status, response);
            co_return;
        });
}

auto vosfs::ui::VosfsClient::send_list_dir_request(std::string path) -> Task<void> {
    raft::ListDirRequest request;
    request.set_token(session_.token);
    request.set_path(std::move(path));
    auto it = raft_clients_.find(leader_id_);
    if (it == raft_clients_.end()) {
        signal_brige_.appendLog(QString::fromStdString(std::format("请求失败，节点 {} 不存在，请检查集群配置", leader_id_)));
        co_return;
    }
    auto& raft_client = it->second;
    co_await raft_client->call_method<raft::ListDirRequest, raft::ListDirResponse>(
        "fs", "ls", request,
        [this](const vrpc::Status& status, const raft::ListDirResponse& response) -> Task<void> {
            co_await this->handle_list_dir_response(status, response);
        });
}

auto vosfs::ui::VosfsClient::send_make_dir_request(std::string parent_path, std::string name) -> Task<void> {
    raft::MakeDirRequest request;
    request.set_token(session_.token);
    request.set_parent_path(std::move(parent_path));
    request.set_name(std::move(name));
    auto it = raft_clients_.find(leader_id_);
    if (it == raft_clients_.end()) {
        signal_brige_.appendLog(QString::fromStdString(std::format("请求失败，节点 {} 不存在，请检查集群配置", leader_id_)));
        co_return;
    }
    auto& raft_client = it->second;
    co_await raft_client->call_method<raft::MakeDirRequest, raft::MakeDirResponse>(
        "fs", "mkdir", request,
        [this](const vrpc::Status& status, const raft::MakeDirResponse& response) -> Task<void> {
            co_await this->handle_make_dir_response(status, response);
        });
}

auto vosfs::ui::VosfsClient::send_prepare_upload_file_request(std::string local_path, std::string current_dir) -> Task<void> {
    static constexpr std::size_t VOSFS_BLOCK_SIZE = 1 * 1024 * 1024;

    auto has_statx = co_await kosio::fs::metadata(local_path);

    if (!has_statx) {
        signal_brige_.appendLog(QString::fromStdString(std::format("获取文件 {} 元数据失败：{}", local_path, has_statx.error())));
        co_return;
    }

    struct statx &stx = has_statx.value();
    uint64_t file_size = stx.stx_size;

    if (file_size <= 0) {
        signal_brige_.appendLog(QString::fromStdString(std::format("无法上传空文件 {}", local_path)));
        co_return;
    }

    raft::PrepareUploadFileRequest request;
    request.set_local_path(local_path);
    auto pos = local_path.find_last_of('/');
    if (pos == std::string::npos) {
        signal_brige_.appendLog("上传文件无效：当前目录无效");
        co_return;
    }
    auto file_name = local_path.substr(pos + 1);
    std::string remote_path;
    if (current_dir == "/") {
        remote_path = current_dir + file_name;
    } else {
        remote_path = current_dir + "/" + file_name;
    }
    request.set_remote_path(std::move(remote_path));
    auto* mutable_blocks = request.mutable_blocks();
    request.set_token(session_.token);
    std::size_t block_id = 0;
    std::size_t offset = 0;
    while (file_size > 0) {
        auto* new_block = mutable_blocks->Add();
        new_block->set_id(block_id++);
        new_block->set_offset(offset);
        if (file_size >= VOSFS_BLOCK_SIZE) {
            offset += VOSFS_BLOCK_SIZE;
            new_block->set_size(VOSFS_BLOCK_SIZE);
            file_size -= VOSFS_BLOCK_SIZE;
        } else {
            offset += file_size;
            new_block->set_size(file_size);
            file_size = 0;
        }
    }

    auto it = raft_clients_.find(leader_id_);
    if (it == raft_clients_.end()) {
        signal_brige_.appendLog(QString::fromStdString(std::format("请求失败，节点 {} 不存在，请检查集群配置", leader_id_)));
        co_return;
    }
    auto& raft_client = it->second;
    co_await raft_client->call_method<raft::PrepareUploadFileRequest, raft::PrepareUploadFileResponse>(
        "fs", "prepareuploadfile", request,
        [this](const vrpc::Status& status, const raft::PrepareUploadFileResponse& response) -> Task<void> {
            co_await this->handle_prepare_upload_file_response(status, response);
        });
}

auto vosfs::ui::VosfsClient::send_upload_block_request(
    const std::string& local_path,
    uint64_t block_id,
    uint64_t ino,
    uint64_t offset,
    uint64_t size,
    uint64_t data_node_id) -> Task<void> {
    raft::UploadBlockRequest request;
    request.set_token(session_.token);
    request.set_block_id(block_id);
    request.set_ino(ino);

    std::ifstream file(local_path, std::ios::binary);
    if (!file.is_open()) {
        signal_brige_.appendLog(QString::fromStdString(std::format("错误：无法打开文件 {}", local_path)));
        co_return;
    }

    std::string block_data;
    block_data.resize(size);
    file.read(block_data.data(), size);
    request.set_data(std::move(block_data));

    auto it = data_clients_.find(data_node_id);
    if (it == data_clients_.end()) {
        signal_brige_.appendLog(QString::fromStdString(std::format("请求失败，节点 {} 不存在，请检查集群配置", leader_id_)));
        co_return;
    }
    auto& data_client = it->second;
    co_await data_client->call_method<raft::UploadBlockRequest, raft::UploadBlockResponse>(
        "fs", "uploadblock", request,
        [this](const vrpc::Status& status, const raft::UploadBlockResponse& response) -> Task<void> {
            co_await this->handle_upload_block_response(status, response);
        });
}

auto vosfs::ui::VosfsClient::send_upload_file_request(uint64_t ino) -> Task<void> {

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
        load_transport_tasks_json();
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
    if (res_status.is_not_leader()) {
        leader_id_ = std::stoull(response.message());
        signal_brige_.appendLog(QString("重定向到集群领导者，请重试"));
        co_return;
    }
    if (res_status.ok()) {
        std::vector<raft::DirEntry> entries;
        for (const auto& entry : response.dir_entries()) {
            entries.push_back(entry);
        }

        std::ranges::sort(entries, [](const auto& a, const auto& b) {
            return a.name() < b.name();
        });

        QVariantList list;

        for (const auto &entry : entries) {
            QVariantMap item;

            item["ino"] = static_cast<qulonglong>(entry.ino());
            item["name"] = QString::fromStdString(entry.name());
            item["path"] = QString::fromStdString(entry.path());
            item["is_dir"] = entry.is_dir();
            item["ctime"] = QString::fromStdString(kosio::util::format_time(entry.ctime()));
            item["mtime"] = QString::fromStdString(kosio::util::format_time(entry.mtime()));

            list.append(item);
        }
        signal_brige_.listDirFinished(QString::fromStdString(response.path()), std::move(list));
    }
    signal_brige_.appendLog(QString::fromStdString(response.message()));
}

auto vosfs::ui::VosfsClient::handle_make_dir_response(
    const vrpc::Status& status,
    const raft::MakeDirResponse& response) -> Task<void> {
    if (!status.ok()) {
        signal_brige_.appendLog(QString::fromStdString(std::string{status.message()}));
        co_return;
    }

    auto res_status = Status{response.status_code()};
    if (res_status.is_not_leader()) {
        leader_id_ = std::stoull(response.message());
        signal_brige_.appendLog(QString("重定向到集群领导者，请重试"));
        co_return;
    }
    if (res_status.ok()) {
        co_await send_list_dir_request(response.parent_path());
    }
    signal_brige_.appendLog(QString::fromStdString(response.message()));
    signal_brige_.makeDirFinished();
}

auto vosfs::ui::VosfsClient::handle_prepare_upload_file_response(
    const vrpc::Status& status,
    const raft::PrepareUploadFileResponse& response) -> Task<void> {
    if (!status.ok()) {
        signal_brige_.appendLog(QString::fromStdString(std::string{status.message()}));
        co_return;
    }

    auto res_status = Status{response.status_code()};
    if (res_status.is_not_leader()) {
        leader_id_ = std::stoull(response.message());
        signal_brige_.appendLog(QString("重定向到集群领导者，请重试"));
        co_return;
    }
    if (res_status.ok()) {
        detail::TransportTask transport_task;
        transport_task.ino = response.blocks().begin()->ino();
        transport_task.total_blocks = response.blocks_size();
        transport_task.done_blocks = 0;
        transport_task.local_path = response.local_path();
        transport_task.remote_path = response.remote_path();

        nlohmann::json task_json;
        task_json["ino"] = transport_task.ino;
        task_json["total_blocks"] = transport_task.total_blocks;
        task_json["done_blocks"] = transport_task.done_blocks;
        task_json["local_path"] = transport_task.local_path;
        task_json["remote_path"] = transport_task.remote_path;

        nlohmann::json blocks_json = nlohmann::json::array();

        for (const auto& block : response.blocks()) {
            detail::BlockInfo block_info;
            block_info.done = false;
            block_info.id = block.id();
            block_info.ino = block.ino();
            block_info.offset = block.offset();
            block_info.size = block.size();

            for (auto& data_node_id : block.data_node_id()) {
                block_info.data_node_ids.push_back(data_node_id);
            }

            // 把 block 转成 json
            nlohmann::json block_json;
            block_json["done"] = block_info.done;
            block_json["id"] = block_info.id;
            block_json["ino"] = block_info.ino;
            block_json["offset"] = block_info.offset;
            block_json["size"] = block_info.size;
            block_json["data_node_ids"] = block_info.data_node_ids;

            blocks_json.push_back(std::move(block_json));
            transport_task.blocks.emplace(block_info.id, std::move(block_info));
        }
        task_json["blocks"] = std::move(blocks_json);
        co_await mutex_.lock();
        auto& upload_tasks_json = json_["upload_tasks"];
        upload_tasks_json.push_back(std::move(task_json));
        auto path = std::to_string(session_.uid) + "-" + "transport_tasks.json";
        std::ofstream f(path);
        f << json_.dump(4);
        f.close();
        upload_tasks_.emplace(transport_task.ino, std::move(transport_task));
        mutex_.unlock();
    }
}

auto vosfs::ui::VosfsClient::handle_upload_block_response(
    const vrpc::Status& status,
    const raft::UploadBlockResponse& response) -> Task<void> {
    if (!status.ok()) {
        signal_brige_.appendLog(QString::fromStdString(std::string{status.message()}));
        co_return;
    }

    auto res_status = Status{response.status_code()};
    if (res_status.ok()) {
        auto& upload_task = upload_tasks_[response.ino()];
        auto& block = upload_task.blocks[response.block_id()];
        block.done = true;
        ++upload_task.done_blocks;
        if (upload_task.done_blocks >= upload_task.total_blocks) {
            tasks_.emplace(send_upload_file_request(upload_task.ino));
        }
        // TODO: 更新进度
    } else {
        // TODO: 重传
    }
}

void vosfs::ui::VosfsClient::load_transport_tasks_json() {
    auto path = std::to_string(session_.uid) + "-" + "transport_tasks.json";
    if (!std::filesystem::exists(path)) {
        nlohmann::json json;
        json["upload_tasks"] = nlohmann::json::array();
        json["download_tasks"] = nlohmann::json::array();
        std::ofstream f(path);
        f << json.dump(4);
        f.close();
    }
    std::ifstream ifs(path);
    auto json = nlohmann::json::parse(ifs);
    std::unordered_map<uint64_t, detail::TransportTask> upload_tasks;
    auto& upload_tasks_json = json["upload_tasks"];
    for (auto& upload_task : upload_tasks_json) {
        auto ino = upload_task["ino"].get<uint64_t>();
        auto total_blocks = upload_task["total_blocks"].get<uint64_t>();
        auto done_blocks = upload_task["done_blocks"].get<uint64_t>();
        auto local_path = upload_task["local_path"].get<std::string>();
        auto remote_path = upload_task["remote_path"].get<std::string>();
        auto& blocks = upload_task["blocks"];
        detail::TransportTask task;
        task.ino = ino;
        task.total_blocks = total_blocks;
        task.done_blocks = done_blocks;
        task.local_path = local_path;
        task.remote_path = remote_path;
        for (auto& block : blocks) {
            detail::BlockInfo block_info;
            block_info.done = block["done"].get<bool>();
            block_info.id = block["id"].get<uint64_t>();
            block_info.ino = block["ino"].get<uint64_t>();
            block_info.offset = block["offset"].get<uint64_t>();
            block_info.size = block["size"].get<uint64_t>();
            block_info.data_node_ids = block["data_node_ids"].get<std::vector<uint64_t>>();
        }
        upload_tasks.emplace(ino, std::move(task));
    }
    std::unordered_map<uint64_t, detail::TransportTask> download_tasks;
    auto& download_tasks_json = json["download_tasks"];
    for (auto& download_task : download_tasks_json) {
        auto ino = download_task["ino"].get<uint64_t>();
        auto total_blocks = download_task["total_blocks"].get<uint64_t>();
        auto done_blocks = download_task["done_blocks"].get<uint64_t>();
        auto local_path = download_task["local_path"].get<std::string>();
        auto remote_path = download_task["remote_path"].get<std::string>();
        auto& blocks = download_task["blocks"];
        detail::TransportTask task;
        task.ino = ino;
        task.total_blocks = total_blocks;
        task.done_blocks = done_blocks;
        task.local_path = local_path;
        task.remote_path = remote_path;
        for (auto& block : blocks) {
            detail::BlockInfo block_info;
            block_info.done = block["done"].get<bool>();
            block_info.id = block["id"].get<uint64_t>();
            block_info.ino = block["ino"].get<uint64_t>();
            block_info.offset = block["offset"].get<uint64_t>();
            block_info.size = block["size"].get<uint64_t>();
            block_info.data_node_ids = block["data_node_ids"].get<std::vector<uint64_t>>();
        }
        download_tasks.emplace(ino, std::move(task));
    }
    json_ = std::move(json);
    upload_tasks_ = std::move(upload_tasks);
    download_tasks_ = std::move(download_tasks);
}

auto vosfs::ui::VosfsClient::do_upload_task(const detail::TransportTask& task) -> Task<void> {
    if (task.done_blocks >= task.total_blocks) {
        tasks_.emplace(send_upload_file_request(task.ino));
        co_return;
    }

    for (const auto& block : task.blocks | std::views::values) {
        if (block.done) {
            continue;
        }

        for (const auto& data_node_id : block.data_node_ids) {
            tasks_.emplace(send_upload_block_request(
                task.local_path,
                block.id,
                block.ino,
                block.offset,
                block.size,
                data_node_id));
        }
    }
}
