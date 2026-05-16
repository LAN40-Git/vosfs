#pragma once
#include <vrpc/net/tcp/tcp_server.hpp>
#include "config.hpp"
#include "log.hpp"
#include "transport.hpp"
#include "state_machine.hpp"
#include "snapshotter.hpp"

namespace vosfs::raft {
using kosio::sync::Mutex;
using kosio::sync::Latch;
using kosio::async::Task;
class RaftNode {
    static constexpr std::string_view SNAPSHOT_DIR = "raft/snapshot";
    static constexpr std::string_view SNAPSHOT_TEMP_FILE_NAME = "snapshot.tmp";
    static constexpr std::string_view SNAPSHOT_FILE_NAME = "snapshot.bin";
    static constexpr std::string_view DB_DIR = "raft/db";

private:
    explicit RaftNode(
        std::unique_ptr<detail::Snapshotter> snapshotter,
        detail::StateMachine state_machine,
        detail::Persister persister,
        detail::RaftLog logs,
        detail::Transport transport,
        HardState hard_state);

public:
    static auto create(const std::string& config_path) -> Result<std::unique_ptr<RaftNode>>;

public:
    [[REMEMBER_CO_AWAIT]]
    auto wait() -> Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto shutdown() -> Task<void>;

private:
    auto election_loop() -> Task<void>;
    auto heartbeat_loop() -> Task<void>;
    auto send_snapshot(uint64_t member_id) -> Task<void>;
    auto apply_entry() -> Task<void>;

private:
    void increase_term_to(uint64_t term);
    void become_leader();


private:
    [[REMEMBER_CO_AWAIT]]
    auto handle_request_vote_request(const RequestVoteRequest& request) -> Task<RequestVoteResponse>;
    [[REMEMBER_CO_AWAIT]]
    auto handle_append_entries_request(const AppendEntriesRequest& request) -> Task<AppendEntriesResponse>;
    [[REMEMBER_CO_AWAIT]]
    auto handle_install_snapshot_request(const InstallSnapshotRequest& request) -> Task<InstallSnapshotResponse>;
    [[REMEMBER_CO_AWAIT]]
    auto handle_list_dir_request(const ListDirRequest& request) -> Task<ListDirResponse>;
    [[REMEMBER_CO_AWAIT]]
    auto handle_make_dir_request(MakeDirRequest& request) -> Task<MakeDirResponse>;
    [[REMEMBER_CO_AWAIT]]
    auto handle_prepare_upload_file_request(PrepareUploadFileRequest& request) -> Task<PrepareUploadFileResponse>;
    [[REMEMBER_CO_AWAIT]]
    auto handle_upload_file_request(UploadFileRequest& request) -> Task<UploadFileResponse>;
    [[REMEMBER_CO_AWAIT]]
    auto handle_prepare_download_file_request(const PrepareDownloadFileRequest& request) -> Task<PrepareDownloadFileResponse>;

private:
    [[REMEMBER_CO_AWAIT]]
    auto handle_request_vote_response(const RequestVoteResponse& response) -> Task<void>;
    [[REMEMBER_CO_AWAIT]]
    auto handle_append_entries_response(const AppendEntriesResponse& response) ->Task<void>;
    [[REMEMBER_CO_AWAIT]]
    auto handle_install_snapshot_response(const InstallSnapshotResponse& response) -> Task<void>;

private:
    static auto suspend_task() -> Task<void> {
        co_await std::suspend_always{};
        co_return;
    }

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
        uint64_t last_included_term) -> InstallSnapshotRequest;

    [[nodiscard]]
    static auto make_install_snapshot_response(
        uint64_t id,
        uint64_t term) -> InstallSnapshotResponse;

    [[nodiscard]]
    static auto make_list_dir_response(
        uint32_t status_code,
        std::string message) -> ListDirResponse;

    [[nodiscard]]
    static auto make_make_dir_response(
        uint32_t status_code,
        std::string message) -> MakeDirResponse;

    [[nodiscard]]
    static auto make_prepare_upload_file_response(
        uint32_t status_code,
        std::string message) -> PrepareUploadFileResponse;

    [[nodiscard]]
    static auto make_upload_file_response(
        uint32_t status_code,
        std::string message,
        uint64_t ino) -> UploadFileResponse;

    [[nodiscard]]
    static auto make_prepare_download_file_response(
        uint32_t status_code,
        std::string message) -> PrepareDownloadFileResponse;

private:
    enum Role { kLeader, kFollower, kCandidate };

    Mutex                    mutex_;
    Latch                    latch_{2};
    std::atomic<bool>        is_shutdown_{false};
    std::atomic<uint64_t>    last_reset_time_{0};
    vrpc::TcpServer          rpc_server_;
    detail::Snapshotter::Ptr snapshotter_;
    std::string              snapshot_data_; // 暂存接收的快照数据
    detail::StateMachine     state_machine_;
    detail::Persister        persister_;
    detail::RaftLog          logs_;
    detail::Transport        transport_;

    /* RaftState from https://raft.github.io/raft.pdf */
    // Persistent state on all servers
    HardState hard_state_;

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