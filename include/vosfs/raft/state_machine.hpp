#pragma once
#include <unordered_map>
#include "raftpb/raft.pb.h"

namespace vosfs::raft::detail {
class StateMachine {
    using InodeMap = std::unordered_map<uint64_t, Inode>;
public:
    explicit StateMachine(InodeMap inodes)
        : inodes_(std::move(inodes)) {
        // 若无快照，则创建根节点
        if (inodes_.empty()) {
            Inode inode;
            inode.set_ino(1);
            inodes_.emplace(1, std::move(inode));
        }
    }

public:
    void apply_entry(const LogEntry& entry);

    [[nodiscard]]
    auto inodes() -> InodeMap& { return inodes_; }

    void set_inodes(InodeMap inodes) {
        inodes_ = std::move(inodes);
    }

private:
    InodeMap inodes_;
};
} // namespace vosfs::raft::detail