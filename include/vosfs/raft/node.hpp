#pragma once
#include <vrpc/net/tcp/tcp_server.hpp>
#include "log.hpp"
#include "transport.hpp"
#include "state_machine.hpp"

namespace vosfs::raft {
class RaftNode {
    using SnapshotContextMap = std::unordered_map<uint64_t, uint64_t>;
private:
    explicit RaftNode(
        Persister&& persister,
        detail::RaftLog&& logs,
        detail::Transport&& transport,
        DataClusterInfo&& data_cluster_info,
        HardState&& hard_state);

public:
    static auto create(std::string_view data_dir) -> kosio::async::Task<Result<std::unique_ptr<RaftNode>>>;

public:
    [[REMEMBER_CO_AWAIT]]
    auto wait() -> kosio::async::Task<void>;

private:
    auto apply_snapshot(Snapshot& snapshot) -> kosio::async::Task<void>;
    auto election_loop() -> kosio::async::Task<void>;
    auto heartbeat_loop() -> kosio::async::Task<void>;
    auto send_snapshot(uint64_t member_id, uint64_t offset) -> kosio::async::Task<void>;

private:
    void increase_term_to(uint64_t term);
    void become_leader();
    void apply_to_state_machine();

private:
    // Raft RPC
    [[REMEMBER_CO_AWAIT]]
    auto handle_request_vote_request(const RequestVoteRequest& request)
        -> kosio::async::Task<RequestVoteResponse>;
    [[REMEMBER_CO_AWAIT]]
    auto handle_append_entries_request(const AppendEntriesRequest& request)
        -> kosio::async::Task<AppendEntriesResponse>;
    [[REMEMBER_CO_AWAIT]]
    auto handle_install_snapshot_request(const InstallSnapshotRequest& request)
        -> kosio::async::Task<InstallSnapshotResponse>;

private:
    [[REMEMBER_CO_AWAIT]]
    auto handle_request_vote_response(const RequestVoteResponse& response) -> kosio::async::Task<void>;
    [[REMEMBER_CO_AWAIT]]
    auto handle_append_entries_response(const AppendEntriesResponse& response) -> kosio::async::Task<void>;
    [[REMEMBER_CO_AWAIT]]
    auto handle_install_snapshot_response(const InstallSnapshotResponse& response) -> kosio::async::Task<void>;

private:
    [[nodiscard]]
    static auto make_request_vote_request(
        uint64_t term,
        uint64_t candidate_id,
        uint64_t last_log_index,
        uint64_t last_log_term) -> RequestVoteRequest;

    [[nodiscard]]
    static auto make_request_vote_response(
        uint64_t id,
        uint64_t term,
        bool vote_granted) -> RequestVoteResponse;

    [[nodiscard]]
    static auto make_append_entries_request(
        uint64_t term,
        uint64_t leader_id,
        uint64_t prev_log_index,
        uint64_t prev_log_term,
        std::span<LogEntry> entries,
        uint64_t leader_commit) -> AppendEntriesRequest;

    [[nodiscard]]
    static auto make_append_entries_response(
        uint64_t id,
        uint64_t term,
        bool success,
        uint64_t last_log_index,
        std::optional<uint64_t> conflict_index) -> AppendEntriesResponse;

    [[nodiscard]]
    static auto make_install_snapshot_request(
        uint64_t term,
        uint64_t leader_id,
        uint64_t last_included_index,
        uint64_t last_included_term,
        uint64_t offset,
        std::string data,
        bool done) -> InstallSnapshotRequest;

    [[nodiscard]]
    static auto make_install_snapshot_response(
        uint64_t id,
        uint64_t term) -> InstallSnapshotResponse;

private:
    enum Role { kLeader, kFollower, kCandidate };

    kosio::sync::Mutex    mutex_;
    kosio::sync::Latch    latch_{2};
    std::atomic<bool>     is_shutdown_{false};
    std::atomic<uint64_t> last_reset_time_{0};
    Persister             persister_;
    StateMachine          state_machine_;
    detail::RaftLog       logs_;
    detail::Transport     transport_;
    const DataClusterInfo data_cluster_info_;

    /* RaftState from https://raft.github.io/raft.pdf */
    // Persistent state on all servers
    HardState          hard_state_;
    std::string        snapshot_data_{};
    SnapshotContextMap snapshot_context_{};

    // Volatile state on all servers
    std::size_t             votes_{0};
    std::atomic<Role>       role_{kFollower};
    uint64_t                commit_index_{0};
    uint64_t                last_applied_{0};
    std::optional<uint64_t> leader_id_{std::nullopt};

    // Volatile state on leaders
    std::unordered_map<uint64_t, uint64_t> next_index_{};
    std::unordered_map<uint64_t, uint64_t> match_index_{};
};
} // namespace vosfs::raft