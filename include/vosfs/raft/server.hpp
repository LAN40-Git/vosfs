#pragma once
#include "vosfs/raft/internal/log.hpp"
#include "vosfs/raft/internal/transport.hpp"
#include "state_machine.hpp"

namespace vosfs::raft {
class RaftServer {
    using SnapshotContextMap = std::unordered_map<uint64_t, uint64_t>;
private:
    explicit RaftServer(
        rpc::RpcServer raft_rpc_server,
        rpc::RpcServer client_rpc_server,
        Persister&& persister,
        detail::RaftLog&& logs,
        detail::Transport&& transport,
        HardState&& hard_state,
        std::string&& snapshot_data);

public:
    static auto create(std::string_view data_dir) -> kosio::async::Task<Result<std::unique_ptr<RaftServer>>>;

public:
    auto run() -> kosio::async::Task<void>;
    auto shutdown() -> kosio::async::Task<void>;

private:
    auto election_loop() -> kosio::async::Task<void>;
    auto heartbeat_loop() -> kosio::async::Task<void>;

private:
    void init();
    void do_election();
    void do_heartbeat();
    void increase_term_to(uint64_t term);
    void become_leader();
    void apply_to_state_machine();
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

    kosio::sync::Mutex    mutex_;
    kosio::sync::Latch    latch_{2}; // 两个循环协程-(election_loop 和 heartbeat_loop)
    std::atomic<bool>     is_shutdown_{false};
    std::atomic<uint64_t> last_reset_time_{0};
    rpc::RpcServer        raft_rpc_server_;
    rpc::RpcServer        client_rpc_server_;
    Persister             persister_;
    StateMachine          state_machine_;
    detail::RaftLog       logs_;
    detail::Transport     transport_;

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