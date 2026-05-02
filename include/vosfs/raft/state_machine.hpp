#pragma once
#include <coroutine>
#include <ranges>
#include <unordered_map>
#include "raftpb/raft.pb.h"

namespace vosfs::raft::detail {
struct PendingRequest {
    std::coroutine_handle<> handle;
    PendingResponse response;
};

class StateMachine {
    using InodeMap = std::unordered_map<uint64_t, Inode>;
    using PendingRequestMap = std::unordered_map<uint64_t, PendingRequest>;
public:
    explicit StateMachine(InodeMap inodes)
        : inodes_(std::move(inodes)) {
        // 若无快照，则创建根节点
        if (inodes_.empty()) {
            Inode inode;
            inode.set_ino(0);
            inode.set_size(0);
            inode.set_nlink(0);
            inodes_.emplace(1, std::move(inode));
        }
        get_next_ino();
    }

public:
    void apply_entry(const LogEntry& entry);
    auto append_request(uint64_t log_index, PendingRequest request) -> PendingRequest&;
    [[nodiscard]]
    auto inodes() -> InodeMap& { return inodes_; }
    void set_inodes(InodeMap inodes);
    void get_next_ino();

private:
    void mkdir(const MakeDirRequest& request, MakeDirResponse* response);

private:
    InodeMap          inodes_;
    uint64_t          next_ino_;
    PendingRequestMap pending_requests_;
};
} // namespace vosfs::raft::detail