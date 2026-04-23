#pragma once
#include <unordered_map>
#include "raftpb/raft.pb.h"

namespace vosfs::raft {
class StateMachine {
    using InodeMap = std::unordered_map<uint64_t, Inode>;
public:
    void apply_entry(const LogEntry& entry);

private:
    InodeMap inodes_;
};
} // namespace vosfs::raft