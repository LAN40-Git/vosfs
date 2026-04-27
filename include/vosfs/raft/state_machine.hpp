#pragma once
#include <unordered_map>
#include "raftpb/raft.pb.h"

namespace vosfs::raft::detail {
class StateMachine {
    using InodeMap = std::unordered_map<uint64_t, Inode>;
public:
    explicit StateMachine(InodeMap inodes)
        : inodes_(std::move(inodes)) {}

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