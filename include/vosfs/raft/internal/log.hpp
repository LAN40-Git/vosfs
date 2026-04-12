#pragma once
#include "vosfs/api/serverpb/raft.pb.h"
#include "vosfs/raft/internal/persister.hpp"

namespace vosfs::raft::detail {
class RaftLog {
    static constexpr std::string LOG_ENTRY_PREFIX = "raft/log/";
private:
    explicit RaftLog(
        Persister& persister,
        SnapshotMetadata&& snapshot_metadata,
        std::vector<LogEntry>&& entries)
        : persister_(persister)
        , snapshot_metadata_(std::move(snapshot_metadata))
        , entries_(std::move(entries)) {}

public:
    static auto create(Persister& persister) -> Result<RaftLog>;

public:
    [[nodiscard]] auto last_included_index() const noexcept -> uint64_t;

    [[nodiscard]] auto last_included_term() const noexcept -> uint64_t;

    [[nodiscard]] auto last_log_index() const noexcept -> uint64_t;

    [[nodiscard]] auto last_log_term() const noexcept -> uint64_t;

    [[nodiscard]] auto get_term(uint64_t index) const -> uint64_t;

    [[nodiscard]] auto get_entry(uint64_t index) const -> LogEntry;

    [[nodiscard]] auto get_entries(uint64_t index, std::size_t size) const -> std::vector<LogEntry>;

    [[nodiscard]] auto get_entries_span(uint64_t index, std::size_t size) const -> std::span<const LogEntry>;

    [[nodiscard]] auto append_entry(LogEntry&& entry) -> Result<void>;

    [[nodiscard]] auto append_entries(const google::protobuf::RepeatedPtrField<LogEntry>& entries) -> Result<void>;

    [[nodiscard]] auto truncate_entries(uint64_t index) -> Result<void>;

private:
    Persister&               persister_;
    SnapshotMetadata         snapshot_metadata_;
    std::vector<LogEntry>    entries_;
};
} // namespace vosfs::raft::detail