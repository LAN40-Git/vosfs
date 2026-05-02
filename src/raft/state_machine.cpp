#include "vosfs/raft/state_machine.hpp"
#include "vosfs/common/status.hpp"
#include <kosio/common/debug.hpp>
#include <kosio/runtime/runtime.hpp>

void vosfs::raft::detail::StateMachine::apply_entry(const LogEntry& entry) {
    auto& pending_request = pending_requests_[entry.index()];
    auto& pending_response = pending_request.response;
    EntryCommand command;
    if (!command.ParseFromString(entry.command())) {
        LOG_FATAL("failed to parse command at entry {}", entry.index());
        pending_request.handle.resume();
        return;
    }

    switch (command.type_case()) {
        case EntryCommand::kMkdir: {
            mkdir(command.mkdir(), pending_response.mutable_mkdir());
            pending_request.handle.resume();
            break;
        }
        default: {
            LOG_FATAL("invalid command at entry {}", entry.index());
            pending_request.handle.resume();
            break;
        }
    }
}

auto vosfs::raft::detail::StateMachine::append_request(
    uint64_t log_index,
    PendingRequest request) -> PendingRequest& {
    return pending_requests_.emplace(log_index, std::move(request)).first->second;
}

void vosfs::raft::detail::StateMachine::set_inodes(InodeMap inodes) {
    inodes_ = std::move(inodes);
    get_next_ino();
}

void vosfs::raft::detail::StateMachine::get_next_ino() {
    uint64_t max_ino{0};
    for (const auto& inode : inodes_ | std::views::values) {
        max_ino = std::max(max_ino, inode.ino());
    }
    next_ino_ = max_ino + 1;
}

void vosfs::raft::detail::StateMachine::mkdir(
    const MakeDirRequest& request, MakeDirResponse* response) {
    auto parent_ino = request.parent_ino();
    auto& name = request.name();

    // 找到 parent inode
    auto it = inodes_.find(parent_ino);
    if (it == inodes_.end()) {
        LOG_FATAL("parent inode not found: {}", parent_ino);
        response->set_message("父目录不存在");
        return;
    }

    auto& parent_inode = it->second;
    if (parent_inode.file_type() == Inode_FileType_FILE) {
        LOG_FATAL("parent inode is not dir: {}", parent_ino);
        response->set_message("父节点不是目录");
        return;
    }

    auto& entries = parent_inode.entries();
    if (entries.contains(name)) {
        response->set_message("目录已存在");
        return;
    }

    Inode new_inode;
    auto ino = next_ino_++;
    new_inode.set_ino(ino);
    // new_inode.set_mode()
    new_inode.set_file_type(Inode_FileType_DIR);
    auto current_ms = kosio::util::current_ms();
    new_inode.set_atime(current_ms);
    new_inode.set_ctime(current_ms);
    new_inode.set_mtime(current_ms);
    new_inode.set_size(0);
    new_inode.set_nlink(0);
    inodes_.emplace(ino, std::move(new_inode));

    // 在 parent inode 中创建该目录
    auto* mutable_entries = parent_inode.mutable_entries();
    mutable_entries->emplace(name, ino);

    response->set_status_code(Status::kOk);
    response->set_message("创建文件夹成功");
    response->set_ino(ino);
    response->set_parent_ino(parent_ino);
    response->set_name(name);
}
