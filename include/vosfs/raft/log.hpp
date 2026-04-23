#pragma once
#include <span>
#include "persister.hpp"

namespace vosfs::raft::detail {
class RaftLog {
private:
    explicit RaftLog(
        uint64_t last_included_index,
        uint64_t last_included_term,
        std::vector<LogEntry>&& entries)
        : last_included_index_(last_included_index)
        , last_included_term_(last_included_term)
        , entries_(std::move(entries)) {}

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

private:
    uint64_t                 last_included_index_;
    uint64_t                 last_included_term_;
    std::vector<LogEntry>    entries_;
};
} // namespace vosfs::raft::detail