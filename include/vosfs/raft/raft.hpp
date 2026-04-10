#pragma once
#include "vosfs/raft/internal/message_factory.hpp"
#include "vosfs/raft/internal/log.hpp"
#include "vosfs/raft/internal/transport.hpp"
#include "vosfs/raft/internal/state_machine.hpp"

namespace vosfs::raft {
class RaftNode {
private:
    // explicit RaftNode() {}

public:
    static auto create() -> kosio::async::Task<Result<std::unique_ptr<RaftNode>>>;

public:
    auto shutdown() -> kosio::async::Task<void>;

private:
    auto election_loop() -> kosio::async::Task<void>;
    auto heartbeat_loop() -> kosio::async::Task<void>;

private:
    void persist_current_term() const;
    void persist_voted_for() const;
    void do_election();
    void do_heartbeat();
    void increase_term_to(uint64_t term);
    void become_leader();
    void apply_to_state_machine();

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

    kosio::sync::Mutex                mutex_;
    std::atomic<bool>                 is_shutdown_{false};
    std::atomic<uint64_t>             last_reset_time_{0};
    detail::Transport                 transport_;
    std::atomic<Role>                 role_{kFollower};
    std::size_t                       votes_{0};
    std::optional<uint64_t>           leader_id_{std::nullopt};
    std::unique_ptr<rpc::RpcProvider> raft_provider_;
    std::unique_ptr<rpc::RpcProvider> client_provider_;

    /* RaftState from https://raft.github.io/raft.pdf */
    // Persistent state on all servers
    std::atomic<uint64_t>    current_term_;
    std::optional<uint64_t>  voted_for_;
    detail::RaftLog          logs_;
    detail::Persister        persister_;
    detail::StateMachine     state_machine_;

    // Volatile state on all servers
    uint64_t commit_index_{0};
    uint64_t last_applied_{0};

    // Volatile state on leaders
    std::unordered_map<uint64_t, uint64_t> next_index_{};
    std::unordered_map<uint64_t, uint64_t> match_index_{};
};
} // namespace vosfs::raft