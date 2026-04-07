#include "vosfs/raft/internal/log.hpp"
#include <kosio/common/debug.hpp>

auto vosfs::raft::detail::RaftLog::create(Persister& persister) -> Result<RaftLog> {
    std::string last_included_index;
    std::string last_included_term;
    auto ret = persister.recover(SNAPSHOT_LAST_INCLUDED_INDEX_KEY, &last_included_index);
    if (!ret) {
        return std::unexpected{ret.error()};
    }
    ret = persister.recover(SNAPSHOT_LAST_INCLUDED_TERM_KEY, &last_included_term);
    if (!ret) {
        return std::unexpected{ret.error()};
    }


}

auto vosfs::raft::detail::RaftLog::last_log_index() const noexcept -> uint64_t {

}

auto vosfs::raft::detail::RaftLog::last_log_term() const noexcept -> uint64_t {

}
