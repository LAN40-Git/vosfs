#include "vosfs/raft/internal/log.hpp"
#include <kosio/common/debug.hpp>

auto vosfs::raft::detail::RaftLog::create(Persister& persister) -> Result<RaftLog> {
    std::string value;
    auto ret = persister.recover(SNAPSHOT_LAST_INCLUDED_INDEX_KEY, &value);
    if (!ret) {
        return std::unexpected{ret.error()};
    }
    auto last_included_index = std::stoull(value);
    ret = persister.recover(SNAPSHOT_LAST_INCLUDED_TERM_KEY, &value);
    if (!ret) {
        return std::unexpected{ret.error()};
    }
    auto last_included_term = std::stoull(value);

    // recover entries
    uint64_t i = last_included_index + 1;
    std::vector<LogEntry> entries;
    while (true) {
        auto key = LOG_ENTRY_PREFIX + std::to_string(i++);
        std::string entry_payload;
        ret = persister.recover(key, &entry_payload);
        if (!ret) {
            break;
        }

        LogEntry entry;
        if (!entry.ParseFromString(entry_payload)) {
            return std::unexpected{make_error(Error::kProtoParseFailed)};
        }
        entries.emplace_back(std::move(entry));
    }

    return RaftLog{last_included_index, last_included_term, std::move(entries), persister};
}

auto vosfs::raft::detail::RaftLog::last_included_index() const noexcept -> uint64_t {
    return last_included_index_;
}

auto vosfs::raft::detail::RaftLog::last_included_term() const noexcept -> uint64_t {
    return last_included_term_;
}

auto vosfs::raft::detail::RaftLog::last_log_index() const noexcept -> uint64_t {
    return last_included_index_ + entries_.size();
}

auto vosfs::raft::detail::RaftLog::last_log_term() const noexcept -> uint64_t {
    if (entries_.empty() && last_log_index() == 0) {
        return last_included_index_;
    }
    if (entries_.empty() && last_log_index() > 0) {
        return last_included_term_;
    }
    return entries_.back().term();
}

auto vosfs::raft::detail::RaftLog::get_term(uint64_t index) const -> uint64_t {
    assert((index > last_included_index_ && index <= last_log_index()) || index == last_included_index_);
    if (index == last_included_index_) {
        return last_included_term_;
    }
    auto arr_idx = index - last_included_index_ - 1;
    return entries_[arr_idx].term();
}

auto vosfs::raft::detail::RaftLog::get_entry(uint64_t index) const -> LogEntry {
    assert(index > last_included_index_ && index <= last_log_index());
    auto arr_idx = index - last_included_index_ - 1;
    return entries_[arr_idx];
}

auto vosfs::raft::detail::RaftLog::get_entries(
    uint64_t index, std::size_t size) const -> std::vector<LogEntry> {
    assert(index > last_included_index_ && index <= last_log_index());

    std::vector<LogEntry> entries;
    auto arr_idx = index - last_included_index_ - 1;

    while (size > 0 && arr_idx < entries_.size()) {
        entries.push_back(entries_[arr_idx++]);
        --size;
    }

    return entries;
}

auto vosfs::raft::detail::RaftLog::get_entries_span(
    uint64_t index, std::size_t size) const -> std::span<const LogEntry> {
    assert(index > last_included_index_ && index <= last_log_index());
    auto arr_idx = index - last_included_index_ - 1;
    return std::span{entries_.begin() + arr_idx, size};
}

auto vosfs::raft::detail::RaftLog::append_entry(LogEntry&& entry) -> Result<void> {
    auto key = log_entry_prefix_ + std::to_string(entry.index());
    auto value = entry.SerializeAsString();
    auto ret = persister_.persist(key, value);
    if (!ret) {
        return ret;
    }
    entries_.emplace_back(std::move(entry));
    // 尝试创建快照
    return ret;
}

auto vosfs::raft::detail::RaftLog::append_entries(const google::protobuf::RepeatedPtrField<LogEntry>& entries) -> Result<void> {
    std::vector<KV> kvs;
    for (const auto& entry : entries) {
        kvs.emplace_back(log_entry_prefix_ + std::to_string(entry.index()), entry.SerializeAsString());
    }
    auto ret = persister_.persist_batch(kvs);
    if (ret) {
        for (const auto& entry : entries) {
            entries_.push_back(entry);
        }
    }
    return ret;
}

auto vosfs::raft::detail::RaftLog::truncate_entries(uint64_t index) -> Result<void> {
    if (index <= last_included_index_ || index > last_log_index()) {
        return std::unexpected{make_error(Error::kInvalidLogIndex)};
    }

    std::vector<std::string> keys;
    for (uint64_t i = index; i <= last_log_index(); ++i) {
        keys.emplace_back(log_entry_prefix_ + std::to_string(i));
    }

    entries_.erase(entries_.begin() + index - last_included_index_ - 1, entries_.end());
    return persister_.truncate_batch(keys);
}
