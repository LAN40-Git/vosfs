#include "vosfs/raft/log.hpp"

auto vosfs::raft::detail::RaftLog::last_included_index() const noexcept -> uint64_t {
    return last_included_index_;
}

auto vosfs::raft::detail::RaftLog::last_included_term() const noexcept -> uint64_t {
    return last_included_term_;
}

void vosfs::raft::detail::RaftLog::load_snapshot(const Snapshot& snapshot) {
    last_included_index_ = snapshot.last_included_index();
    last_included_term_ = snapshot.last_included_term();
}

auto vosfs::raft::detail::RaftLog::last_log_index() const noexcept -> uint64_t {
    return last_included_index() + entries_.size();
}

auto vosfs::raft::detail::RaftLog::last_log_term() const noexcept -> uint64_t {
    if (entries_.empty() && last_log_index() == 0) {
        return 0;
    }
    if (entries_.empty() && last_log_index() > 0) {
        return last_included_term();
    }
    return entries_.back().term();
}

auto vosfs::raft::detail::RaftLog::get_term(uint64_t index) const -> uint64_t {
    assert((index > last_included_index() && index <= last_log_index()) || index == last_included_index());
    if (index == last_included_index()) {
        return last_included_term();
    }
    auto arr_idx = index - last_included_index() - 1;
    return entries_[arr_idx].term();
}

auto vosfs::raft::detail::RaftLog::get_entry(uint64_t index) const -> LogEntry {
    assert(index > last_included_index() && index <= last_log_index());
    auto arr_idx = index - last_included_index() - 1;
    return entries_[arr_idx];
}

auto vosfs::raft::detail::RaftLog::get_entries(
    uint64_t index, std::size_t size) const -> std::vector<LogEntry> {
    assert(index > last_included_index() && index <= last_log_index());

    std::vector<LogEntry> entries;
    auto arr_idx = index - last_included_index() - 1;

    while (size > 0 && arr_idx < entries_.size()) {
        entries.push_back(entries_[arr_idx++]);
        --size;
    }

    return entries;
}

auto vosfs::raft::detail::RaftLog::get_entries_span(
    uint64_t index, std::size_t size) const -> std::span<const LogEntry> {
    assert(index > last_included_index() && index <= last_log_index());
    auto arr_idx = static_cast<int64_t>(index - last_included_index() - 1);
    return std::span{entries_.begin() + arr_idx, size};
}

void vosfs::raft::detail::RaftLog::append_entry(LogEntry&& entry) {
    entries_.emplace_back(std::move(entry));
}

void vosfs::raft::detail::RaftLog::append_entries(const google::protobuf::RepeatedPtrField<LogEntry>& entries){
    for (const auto& entry : entries) {
        entries_.push_back(entry);
    }
}

void vosfs::raft::detail::RaftLog::truncate_entries_before(uint64_t index) {
    assert(index > last_included_index() && index <= last_log_index());
    auto arr_idx = static_cast<int64_t>(index - last_included_index() - 1);
    entries_.erase(entries_.begin() + arr_idx, entries_.end());
}
