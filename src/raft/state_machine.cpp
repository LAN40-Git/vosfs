#include "vosfs/raft/state_machine.hpp"
#include "vosfs/common/status.hpp"
#include <kosio/common/debug.hpp>
#include <kosio/runtime/runtime.hpp>

vosfs::raft::detail::StateMachine::StateMachine(const Config& config) {
    for (auto& node : config.data_nodes | std::views::values) {
        data_nodes_.emplace(node.id, NodeInfo(node.id, node.name, node.host, node.port));
    }

    Inode root_inode;
    root_inode.set_ino(0);
    root_inode.set_is_dir(true);
    root_inode.set_size(4096);
    root_inode.set_nlink(2);

    DirEntry root_entry;
    root_entry.set_ino(root_inode.ino());
    root_entry.set_name("/");
    root_entry.set_is_dir(true);
    root_entry.set_path("/");
    inodes_.emplace(root_inode.ino(), std::move(root_inode));
    dir_entries_.emplace(root_entry.path(), std::move(root_entry));

    uint64_t max_ino{0};
    for (const auto& inode : inodes_ | std::views::values) {
        max_ino = std::max(max_ino, inode.ino());
    }
    next_ino_ = max_ino + 1;
}

void vosfs::raft::detail::StateMachine::load_snapshot(const Snapshot& snapshot) {
    inodes_.clear();
    dir_entries_.clear();
    for (const auto& inode : snapshot.inodes()) {
        inodes_.emplace(inode.ino(), inode);
    }
    for (const auto& dir_entry : snapshot.dir_entries()) {
        dir_entries_.emplace(dir_entry.path(), dir_entry);
    }

    if (inodes_.empty() && dir_entries_.empty()) {
        Inode root_inode;
        root_inode.set_ino(0);
        root_inode.set_is_dir(true);
        root_inode.set_size(4096);
        root_inode.set_nlink(2);

        DirEntry root_entry;
        root_entry.set_ino(root_inode.ino());
        root_entry.set_name("/");
        root_entry.set_is_dir(true);
        root_entry.set_path("/");
        inodes_.emplace(root_inode.ino(), std::move(root_inode));
        dir_entries_.emplace(root_entry.path(), std::move(root_entry));
    }

    uint64_t max_ino{0};
    for (const auto& inode : inodes_ | std::views::values) {
        max_ino = std::max(max_ino, inode.ino());
    }
    next_ino_ = max_ino + 1;
}

void vosfs::raft::detail::StateMachine::take_snapshot(Snapshot& snapshot) {
    for (const auto& inode : inodes_ | std::views::values) {
        snapshot.add_inodes()->CopyFrom(inode);
    }
    for (const auto& dir_entry : dir_entries_ | std::views::values) {
        snapshot.add_dir_entries()->CopyFrom(dir_entry);
    }
}

void vosfs::raft::detail::StateMachine::apply_entry(const LogEntry& entry) {
    auto& pending_request = pending_requests_[entry.index()];
    auto& pending_response = pending_request.response;
    EntryCommand command;
    if (!command.ParseFromString(entry.command())) {
        LOG_FATAL("failed to parse command at entry {}", entry.index());
        if (pending_request.handle) {
            pending_request.handle.resume();
        }
        return;
    }

    switch (command.type_case()) {
        case EntryCommand::kMkdir: {
            mkdir(command.mkdir(), pending_response.mutable_mkdir());
            if (pending_request.handle) {
                pending_request.handle.resume();
            }
            break;
        }
        default: {
            LOG_FATAL("invalid command at entry {}", entry.index());
            if (pending_request.handle) {
                pending_request.handle.resume();
            }
            break;
        }
    }
}

void vosfs::raft::detail::StateMachine::apply_entry_no_response(const LogEntry& entry) {
    EntryCommand command;
    if (!command.ParseFromString(entry.command())) {
        LOG_FATAL("failed to parse command at entry {}", entry.index());
        return;
    }

    switch (command.type_case()) {
        case EntryCommand::kMkdir: {
            mkdir(command.mkdir());
            break;
        }
        default: {
            LOG_FATAL("invalid command at entry {}", entry.index());
            break;
        }
    }
}

auto vosfs::raft::detail::StateMachine::append_request(
    uint64_t log_index,
    PendingRequest request) -> PendingRequest& {
    return pending_requests_.emplace(log_index, std::move(request)).first->second;
}

void vosfs::raft::detail::StateMachine::ls(const ListDirRequest& request, ListDirResponse& response) {
    // 判断路径是否存在
    auto& path = request.path();
    auto it = dir_entries_.find(path);
    if (it == dir_entries_.end()) {
        response.set_status_code(Status::kInvalidArgument);
        response.set_message("路径不存在");
        return;
    }

    // 判断节点是不是目录
    if (!it->second.is_dir()) {
        response.set_status_code(Status::kInternal);
        response.set_message("节点不是目录");
        return;
    }

    // 获取节点
    auto ino = it->second.ino();
    auto inode_it = inodes_.find(ino);
    if (inode_it == inodes_.end()) {
        response.set_status_code(Status::kInvalidArgument);
        response.set_message("节点不存在");
        LOG_FATAL("inode not exist with path: {}", path);
        return;
    }

    // 获取节点所有子目录
    auto& dir_entries = inode_it->second.dir_entries();
    response.set_status_code(Status::kOk);
    response.set_message(std::format("显示目录：{}", path));
    response.set_path(path);
    response.mutable_dir_entries()->CopyFrom(dir_entries);
}

void vosfs::raft::detail::StateMachine::prepare_upload_file(
    PrepareUploadFileRequest& request,
    PrepareUploadFileResponse& response) {

}

void vosfs::raft::detail::StateMachine::mkdir(const MakeDirRequest& request) {
    auto& parent_path = request.parent_path();
    auto& name = request.name();
    auto timestamp = request.timestamp();

    // 判断父目录是否存在
    auto parent_it = dir_entries_.find(parent_path);
    if (parent_it == dir_entries_.end()) {
        return;
    }

    // 获取父节点
    auto parent_inode_it = inodes_.find(parent_it->second.ino());
    if (parent_inode_it == inodes_.end()) {
        LOG_FATAL("inode not exist with path: {}", parent_path);
        return;
    }

    auto& parent_inode = parent_inode_it->second;
    // 判断父节点是否为目录
    if (!parent_inode.is_dir()) {
        return;
    }

    // 判断目录名合法性
    if (name.empty() || name.contains('/')) {
        return;
    }

    std::string path;

    if (parent_path == "/") {
        path = parent_path + name;
    } else {
        path = parent_path + '/' + name;
    }

    // 判断目录是否存在
    if (dir_entries_.contains(path)) {
        return;
    }

    auto new_ino = next_ino_++;
    Inode new_inode;
    new_inode.set_ino(new_ino);
    new_inode.set_is_dir(true);
    new_inode.set_size(4096);
    new_inode.set_ctime(timestamp);
    new_inode.set_mtime(timestamp);
    new_inode.set_nlink(2);

    DirEntry new_entry;
    new_entry.set_ino(new_ino);
    new_entry.set_name(name);
    new_entry.set_is_dir(true);
    new_entry.set_path(path);
    new_entry.set_size(4096);
    new_entry.set_ctime(timestamp);
    new_entry.set_mtime(timestamp);

    parent_inode.mutable_dir_entries()->Add()->CopyFrom(new_entry);
    inodes_.emplace(new_ino, std::move(new_inode));
    dir_entries_.emplace(path, std::move(new_entry));
}

void vosfs::raft::detail::StateMachine::mkdir(
    const MakeDirRequest& request, MakeDirResponse* response) {
    auto& parent_path = request.parent_path();
    auto& name = request.name();
    auto timestamp = request.timestamp();

    // 判断父目录是否存在
    auto parent_it = dir_entries_.find(parent_path);
    if (parent_it == dir_entries_.end()) {
        response->set_status_code(Status::kInvalidArgument);
        response->set_message("父目录不存在");
        return;
    }

    // 获取父节点
    auto parent_inode_it = inodes_.find(parent_it->second.ino());
    if (parent_inode_it == inodes_.end()) {
        response->set_status_code(Status::kInvalidArgument);
        response->set_message("父节点不存在");
        LOG_FATAL("inode not exist with path: {}", parent_path);
        return;
    }

    auto& parent_inode = parent_inode_it->second;
    // 判断父节点是否为目录
    if (!parent_inode.is_dir()) {
        response->set_status_code(Status::kInvalidArgument);
        response->set_message("父节点不是目录");
        return;
    }

    // 判断目录名合法性
    if (name.empty() || name.contains('/')) {
        response->set_status_code(Status::kInvalidArgument);
        response->set_message("非法目录名，不能为空或包含'/'");
        return;
    }

    std::string path;

    if (parent_path == "/") {
        path = parent_path + name;
    } else {
        path = parent_path + '/' + name;
    }

    // 判断目录是否存在
    if (dir_entries_.contains(path)) {
        response->set_status_code(Status::kInvalidArgument);
        response->set_message("目录已存在");
        return;
    }

    auto new_ino = next_ino_++;
    Inode new_inode;
    new_inode.set_ino(new_ino);
    new_inode.set_is_dir(true);
    new_inode.set_size(4096);
    new_inode.set_ctime(timestamp);
    new_inode.set_mtime(timestamp);
    new_inode.set_nlink(2);

    DirEntry new_entry;
    new_entry.set_ino(new_ino);
    new_entry.set_name(name);
    new_entry.set_is_dir(true);
    new_entry.set_path(path);
    new_entry.set_size(4096);
    new_entry.set_ctime(timestamp);
    new_entry.set_mtime(timestamp);

    parent_inode.mutable_dir_entries()->Add()->CopyFrom(new_entry);
    inodes_.emplace(new_ino, std::move(new_inode));
    dir_entries_.emplace(path, std::move(new_entry));
    response->set_status_code(Status::kOk);
    response->set_message(std::format("创建目录成功：{}", path));
    response->set_parent_path(parent_path);
}
