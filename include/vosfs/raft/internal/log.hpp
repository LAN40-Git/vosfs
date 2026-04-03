#pragma once
#include "vosfs/api/serverpb/raft.pb.h"

namespace vosfs::raft::detail {
class RaftLog {
private:

public:
    [[nodiscard]] auto last_log_index() const noexcept -> uint64_t;

    [[nodiscard]] auto last_log_term() const noexcept -> uint64_t;

private:
    std::vector<LogEntry> entries_;
};
} // namespace vosfs::raft::detail