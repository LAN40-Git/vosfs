#pragma once
#include "vosfs/raft/internal/transport.hpp"

namespace vosfs::raft {
class RaftNode {
public:


private:
    auto start_election_timeout() -> kosio::async::Task<void>;

private:
    enum Role { kLeader, kFollower, kCandidate };

    kosio::sync::Mutex      mutex_;
    std::atomic<bool>       is_shutdown_{false};
    std::atomic<uint64_t>   last_reset_time_{0};
    detail::Transport       transport_;
    std::atomic<Role>       role_{kFollower};
    std::size_t             votes_{0};
    std::optional<uint64_t> leader_id_{std::nullopt};

    /* RaftState from https://raft.github.io/raft.pdf */
    // Persistent state on all servers
    std::atomic<uint64_t>    current_term_;
    std::optional<uint64_t>  voted_for_;
    // detail::RaftLog          logs_;

    // Volatile state on all servers
    uint64_t commit_index_{0};
    uint64_t last_applied_{0};

    // Volatile state on leaders
    std::unordered_map<uint64_t, uint64_t> next_index_{};
    std::unordered_map<uint64_t, uint64_t> match_index_{};
};
} // namespace vosfs::raft