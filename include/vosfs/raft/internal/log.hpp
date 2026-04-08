#pragma once
#include "vosfs/api/serverpb/raft.pb.h"
#include "vosfs/raft/internal/persister.hpp"

namespace vosfs::raft::detail {
class RaftLog {
private:
    explicit RaftLog(uint64_t start_index, std::vector<LogEntry>&& entries, Persister& persister)
        : start_index_(start_index)
        , entries_(std::move(entries))
        , persister_(persister) {}

public:
    static auto create(Persister& persister) -> Result<RaftLog>;

public:
    [[nodiscard]] auto last_log_index() const noexcept -> uint64_t;

    [[nodiscard]] auto last_log_term() const noexcept -> uint64_t;

private:
    uint64_t              start_index_;
    std::vector<LogEntry> entries_;
    Persister&            persister_;
};
} // namespace vosfs::raft::detail