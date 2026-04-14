#pragma once
#include "vosfs/api/serverpb/raft.pb.h"
#include "vosfs/rpc/types.hpp"

namespace vosfs::raft::detail {
class MessageFactory {
public:
    enum RequestType {
        kRequestVoteRequest,
    };

    enum ResponseType {
        kRequestVoteResponse,
    };

public:
    MessageFactory() = delete;

public:
    [[nodiscard]]
    static auto make_request_vote_request(
        uint64_t term,
        uint64_t candidate_id,
        uint64_t last_log_index,
        uint64_t last_log_term) -> RequestVoteRequest;

    [[nodiscard]]
    static auto make_request_vote_response(
        std::span<char> resp_payload,
        uint64_t id,
        uint64_t term,
        bool vote_granted) -> rpc::RpcResult;

    [[nodiscard]]
    static auto make_append_entries_request(
        uint64_t term,
        uint64_t leader_id,
        uint64_t prev_log_index,
        uint64_t prev_log_term,
        std::vector<LogEntry>& entries,
        uint64_t leader_commit) -> AppendEntriesRequest;

    [[nodiscard]]
    static auto make_append_entries_response(
        std::span<char> resp_payload,
        uint64_t id,
        uint64_t term,
        bool success,
        uint64_t last_log_index,
        std::optional<uint64_t> conflict_index = std::nullopt) -> rpc::RpcResult;

    [[nodiscard]]
    static auto make_install_snapshot_request(
        uint64_t term,
        uint64_t leader_id,
        uint64_t last_included_index,
        uint64_t last_included_term,
        uint64_t offset,
        std::string&& data,
        bool done = false) -> InstallSnapshotRequest;

    [[nodiscard]]
    static auto make_install_snapshot_response(
        std::span<char> resp_payload,
        uint64_t term) -> rpc::RpcResult;
};
} // namespace vosfs::raft::detail