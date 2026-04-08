#include "vosfs/raft/internal/log.hpp"
#include <kosio/common/debug.hpp>

auto vosfs::raft::detail::RaftLog::create(Persister& persister) -> Result<RaftLog> {
    std::string value;
    auto ret = persister.recover(SNAPSHOT_LAST_INCLUDED_INDEX_KEY, &value);
    if (!ret) {
        return std::unexpected{ret.error()};
    }

    uint64_t last_included_index = std::stoull(value);
    uint64_t start_index = last_included_index + 1;

    // recover entries
    uint64_t i = start_index;
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

    return RaftLog{start_index, std::move(entries), persister};
}

auto vosfs::raft::detail::RaftLog::last_log_index() const noexcept -> uint64_t {
    return start_index_ + entries_.size() - 1;
}

auto vosfs::raft::detail::RaftLog::last_log_term() const noexcept -> uint64_t {
    if (entries_.empty() && last_log_index() == 0) {
        return 0;
    }
    if (entries_.empty() && last_log_index() > 0) {
        std::string value;
        auto ret = persister_.recover(SNAPSHOT_LAST_INCLUDED_TERM_KEY, &value);
        if (!ret) {
            LOG_ERROR("last_log_term recover faile{}", ret.error());
            std::abort();
        }

        return std::stoull(value);
    }
    return entries_.back().term();
}
