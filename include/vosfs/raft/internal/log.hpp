#pragma once
#include <span>
#include "raft.pb.h"
#include "vosfs/raft/persister.hpp"

namespace vosfs::raft::detail {
class RaftLog {
private:
    explicit RaftLog(
        SnapshotMetadata&& snapshot_metadata,
        std::vector<LogEntry>&& entries)
        : snapshot_metadata_(std::move(snapshot_metadata))
        , entries_(std::move(entries)) {}

public:
    static auto create(const Persister& persister) -> Result<RaftLog>;

public:
    [[nodiscard]] auto last_included_index() const noexcept -> uint64_t;

    [[nodiscard]] auto last_included_term() const noexcept -> uint64_t;

    [[nodiscard]] auto last_log_index() const noexcept -> uint64_t;

    [[nodiscard]] auto last_log_term() const noexcept -> uint64_t;

    [[nodiscard]] auto get_term(uint64_t index) const -> uint64_t;

    [[nodiscard]] auto get_entry(uint64_t index) const -> LogEntry;

    [[nodiscard]] auto get_entries(uint64_t index, std::size_t size) const -> std::vector<LogEntry>;

    [[nodiscard]] auto get_entries_span(uint64_t index, std::size_t size) const -> std::span<const LogEntry>;

    void append_entry(LogEntry&& entry);

    void append_entries(const google::protobuf::RepeatedPtrField<LogEntry>& entries);

    void truncate_entries_before(uint64_t index);

    void apply_snapshot(const Snapshot& snapshot);

private:
    SnapshotMetadata         snapshot_metadata_;
    std::vector<LogEntry>    entries_;
};
} // namespace vosfs::raft::detail