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
};
} // namespace vosfs::raft