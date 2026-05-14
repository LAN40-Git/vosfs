#include <filesystem>
#include "vosfs/storage/node.hpp"
#include "vosfs/common/util/jwt.hpp"
#include "vosfs/common/status.hpp"

auto vosfs::storage::DataNode::wait() -> kosio::async::Task<void> {
    if (!std::filesystem::create_directories(block_dir_)) {
        throw std::runtime_error("failed to create directory");
    }
    auto rpc_server = vrpc::TcpServer{host_, port_};
    co_await rpc_server
        .register_method<raft::UploadBlockRequest, raft::UploadBlockResponse>(
            "user", "register",
            [this](const raft::UploadBlockRequest& request) -> kosio::async::Task<RegisterUserResponse> {
                co_return co_await this->handle_upload_block_request(request);
            })
        // .register_method<DeleteUserRequest, DeleteUserResponse>(
        //     "user", "delete",
        //     [this](const DeleteUserRequest& request) -> kosio::async::Task<DeleteUserResponse> {
        //         co_return co_await this->handle_delete_user_request(request);
        //     })
        // .register_method<LoginUserByNameRequest, LoginUserByNameResponse>(
        //     "user", "loginbyname",
        //     [this](const LoginUserByNameRequest& request) -> kosio::async::Task<LoginUserByNameResponse> {
        //         co_return co_await this->handle_login_user_by_user_name_request(request);
        //     })
        .wait();
}

auto vosfs::storage::DataNode::handle_upload_block_request(
    const raft::UploadBlockRequest& request) -> kosio::async::Task<raft::UploadBlockResponse> {
    auto& token = request.token();
    auto block_id = request.block_id();
    auto ino = request.ino();
    auto& data = request.data();

    // 校验 token
    if (!util::vertify_token(token)) {
        co_return make_upload_block_response(Status::kPermissionDenied, "无效 Token", request.block_id(), request.ino());
    }

    std::string block_path;
    auto block_name = std::to_string(block_id) + "-" + std::to_string(ino);
    if (block_dir_ == "/") {
        block_path = block_dir_ + block_name;
    } else {
        block_path = block_dir_ + "/" + block_name;
    }

    auto file = co_await kosio::fs::File::create(block_path);
    if (!file) {
        co_return make_upload_block_response(Status::kInternal, "分片创建失败", block_id, ino);
    }

    auto write_ret = co_await file.value().write(data);
    if (!write_ret) {
        co_return make_upload_block_response(Status::kInternal, "分片写入失败", block_id, ino);
    }

    co_await file.value().fsync(0);

    co_return make_upload_block_response(Status::kOk, "分片上传成功", block_id, ino);
}

auto vosfs::storage::DataNode::make_upload_block_response(
    uint32_t status_code,
    std::string message,
    uint64_t block_id,
    uint64_t ino) const -> raft::UploadBlockResponse {
    raft::UploadBlockResponse response;
    response.set_status_code(status_code);
    response.set_message(std::move(message));
    response.set_block_id(block_id);
    response.set_ino(ino);
    return response;
}
