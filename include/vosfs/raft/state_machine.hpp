#pragma once
#include <span>
#include <coroutine>
#include <unordered_map>
#include "vosfs/api/serverpb/raft.pb.h"
#include "vosfs/rpc/result.hpp"

namespace vosfs::raft {
class StateMachine {
public:
    void apply(const LogEntry& entry);
    void apply_snapshot(const Snapshot& snapshot);

private:
    std::unordered_map<uint64_t, Inode> inodes_;
};
} // namespace vosfs::raft