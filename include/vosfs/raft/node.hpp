#pragma once
#include "vosfs/raft/internal/log.hpp"
#include "vosfs/raft/internal/transport.hpp"
#include "state_machine.hpp"

namespace vosfs::raft {
class RaftNode {
private:
    // explicit RaftNode() {}

public:
    static auto create(std::string_view data_dir) -> kosio::async::Task<Result<std::unique_ptr<RaftNode>>>;

public:
    auto shutdown() -> kosio::async::Task<void>;

private:
    auto election_loop() -> kosio::async::Task<void>;

    auto heartbeat_loop() -> kosio::async::Task<void>;

private:
    [[REMEMBER_CO_AWAIT]]
    auto do_election() -> kosio::async::Task<void>;

    void do_heartbeat();

    [[REMEMBER_CO_AWAIT]]
    auto increase_term_to(uint64_t term) -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto become_leader() -> kosio::async::Task<void>;

    void apply_to_state_machine();

    [[REMEMBER_CO_AWAIT]]
    auto persist_hard_state() -> kosio::async::Task<void>;

    void send_snapshot(uint64_t member_id, uint64_t offset);

private:
    [[REMEMBER_CO_AWAIT]]
    auto handle_request_vote_request(std::string_view req_payload, std::span<char> resp_payload)
        -> kosio::async::Task<rpc::RpcResult>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_append_entries_request(std::string_view req_payload, std::span<char> resp_payload)
        -> kosio::async::Task<rpc::RpcResult>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_install_snapshot_request(std::string_view req_payload, std::span<char> resp_payload)
        -> kosio::async::Task<rpc::RpcResult>;

private:
    [[REMEMBER_CO_AWAIT]]
    auto handle_request_vote_response(std::string_view resp_payload) -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_append_entries_response(std::string_view resp_payload) -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_install_snapshot_response(std::string_view resp_payload) -> kosio::async::Task<void>;

private:
    enum Role { kLeader, kFollower, kCandidate };
    using RpcServer = std::unique_ptr<rpc::RpcProvider>;
    using SnapshotContextMap = std::unordered_map<uint64_t, uint64_t>;

    kosio::sync::Mutex    mutex_;
    std::atomic<bool>     is_shutdown_{false};
    std::atomic<uint64_t> last_reset_time_{0};
    std::string           snapshot_data_{};
    SnapshotContextMap    snapshot_context_{};
    Persister             persister_;
    StateMachine          state_machine_;
    detail::RaftLog       logs_;
    detail::Transport     transport_;
    RpcServer             raft_rpc_server_;
    RpcServer             client_rpc_server_;

    /* RaftState from https://raft.github.io/raft.pdf */
    // Persistent state on all servers
    HardState               hard_state_;

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