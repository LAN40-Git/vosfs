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
    std::string log_entry_prefix = std::string{LOG_ENTRY_PREFIX};
    while (true) {
        auto key = log_entry_prefix + std::to_string(i++);
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
        return 0;
    }
    if (entries_.empty() && last_log_index() > 0) {
        return last_included_term_;
    }
    return entries_.back().term();
}

auto vosfs::raft::detail::RaftLog::get_term(uint64_t index) const -> uint64_t {
    if (index == last_included_index_) {
        return last_included_term_;
    } else if (index > last_included_index_ && index <= last_log_index()) {
        auto arr_idx = index - last_included_index_ - 1;
        return entries_[arr_idx].term();
    } else {
        LOG_FATAL("get_term: index {} out of range [{}, {}]",
                  index, last_included_index_, last_log_index());
        std::abort();
    }
}

auto vosfs::raft::detail::RaftLog::get_entry(uint64_t index) const -> LogEntry {
    if (index > last_included_index_ && index <= last_log_index()) {
        auto arr_idx = index - last_included_index_ - 1;
        return entries_[arr_idx];
    } else {
        LOG_FATAL("get_entry: index {} out of range [{}, {}]", index, last_included_index_, last_log_index());
        std::abort();
    }
}

auto vosfs::raft::detail::RaftLog::get_entries(
    uint64_t index, std::size_t size) const -> std::vector<LogEntry> {
    if (index > last_included_index_ && index <= last_log_index()) {
        std::vector<LogEntry> entries;
        auto arr_idx = index - last_included_index_ - 1;

        while (size > 0 && arr_idx < entries_.size()) {
            entries.push_back(entries_[arr_idx++]);
            --size;
        }

        return entries;
    } else if (index <= last_included_index_) {
        LOG_FATAL("get_entries: index {} is in snapshot", index);
        std::abort();
    } else {
        return {};
    }
}
