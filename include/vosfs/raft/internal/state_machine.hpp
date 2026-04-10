#pragma once
#include <span>
#include <coroutine>
#include <cstdint>
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
public:
    void apply(std::span<const LogEntry> entries);

private:

private:
    std::unordered_map<uint64_t, ResponseContext> pending_map_;
};
} // namespace vosfs::raft::detail