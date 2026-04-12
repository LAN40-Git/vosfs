#pragma once
#include <span>
#include <coroutine>
#include <unordered_map>
#include "vosfs/api/serverpb/raft.pb.h"
#include "vosfs/rpc/result.hpp"

namespace vosfs::raft::detail {
struct ResponseContext {
    std::coroutine_handle<> handle;
    std::span<char>         resp_payload;
    rpc::RpcResult          result;
};

class StateMachine {
    using PendingMap = std::unordered_map<uint64_t, ResponseContext>;
public:
    void apply(std::span<const LogEntry> entries);

private:
    PendingMap pending_map_;
};
} // namespace vosfs::raft::detail