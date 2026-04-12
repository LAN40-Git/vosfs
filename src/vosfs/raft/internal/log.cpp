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
    std::vector<std::string> keys;
    std::vector<LogEntry> entries;
    while (true) {
        auto key = LOG_ENTRY_PREFIX + std::to_string(i++);
        std::string entry_payload;
        ret = persister.recover(key, &entry_payload);
        if (!ret) {
            break;
        }
        keys.emplace_back(std::move(key));

        LogEntry entry;
        if (!entry.ParseFromString(entry_payload)) {
            return std::unexpected{make_error(Error::kProtoParseFailed)};
        }
        entries.emplace_back(std::move(entry));
    }

    return RaftLog{last_included_index, last_included_term, std::move(keys), std::move(entries), persister};
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
    auto key = LOG_ENTRY_PREFIX + std::to_string(entry.index());
    auto value = entry.SerializeAsString();
    auto ret = persister_.persist(key, value);
    if (!ret) {
        return ret;
    }
    keys_.emplace_back(std::move(key));
    entries_.emplace_back(std::move(entry));
    // 尝试创建快照
    return ret;
}

auto vosfs::raft::detail::RaftLog::append_entries(const google::protobuf::RepeatedPtrField<LogEntry>& entries) -> Result<void> {
    std::vector<KV> kvs;
    for (const auto& entry : entries) {
        kvs.emplace_back(LOG_ENTRY_PREFIX + std::to_string(entry.index()), entry.SerializeAsString());
    }
    auto ret = persister_.persist_batch(kvs);
    if (ret) {
        for (auto& kv : kvs) {
            keys_.emplace_back(std::move(kv.key));
        }
        for (const auto& entry : entries) {
            entries_.push_back(entry);
        }
    }
    return ret;
}

auto vosfs::raft::detail::RaftLog::truncate_entries(uint64_t index) -> Result<void> {
    assert(index > last_included_index_ && index <= last_log_index());
    auto arr_idx = index - last_included_index_ - 1;
    auto ret = persister_.truncate_batch(keys_, arr_idx, entries_.size() - arr_idx + 1);
    if (ret) {
        keys_.erase(keys_.begin() + arr_idx, keys_.end());
        entries_.erase(entries_.begin() + arr_idx, entries_.end());
    }
    return ret;
}

void vosfs::raft::detail::RaftLog::try_create_snapshot() {
    if (entries_.size() < SNAPSHOT_INTERVAL) {
        return;
    }

    // 这里的断言假设每次创建快照都不出错，因为概率太小，处理非常麻烦
    assert(entries_.size() == SNAPSHOT_INTERVAL);

    auto last_included_index = last_included_index_ + SNAPSHOT_INTERVAL;
    auto last_included_term = get_term(last_included_index);

    std::vector<KV> kvs;
    kvs.emplace_back(std::string{SNAPSHOT_LAST_INCLUDED_INDEX_KEY}, std::to_string(last_included_index));
    kvs.emplace_back(std::string{SNAPSHOT_LAST_INCLUDED_TERM_KEY}, std::to_string(last_included_term));
    if (auto ret = persister_.persist_batch(kvs); !ret) {
        LOG_ERROR("create snapshot failed.");
        return;
    }

    if (auto ret = persister_.create_snapshot(last_included_index, last_included_term, keys_); !ret) {
        LOG_ERROR("{}", ret.error());
        // TODO: 回滚元数据（发生的可能性太小，暂不处理）
        return;
    }

    // 更新元数据，清理内存中的日志条目
    last_included_index_ = last_included_index;
    last_included_term_ = last_included_term;
    keys_.resize(0);
    entries_.resize(0);
}
