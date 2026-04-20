#pragma once
#include <unordered_map>
#include "raftpb/raft.pb.h"

namespace vosfs::raft {
class StateMachine {
    using InodeMap = std::unordered_map<uint64_t, Inode>;
public:
    void apply(const LogEntry& entry);
    void apply_snapshot(const Snapshot& snapshot);
    auto inodes() const -> InodeMap { return inodes_; }

private:
    InodeMap inodes_;
};
} // namespace vosfs::raft