#include "vosfs/raft/internal/message_factory.hpp"

auto vosfs::raft::detail::MessageFactory::make_request_vote_request(
    uint64_t term, uint64_t candidate_id,
    uint64_t last_log_index, uint64_t last_log_term) -> RequestVoteRequest {
    RequestVoteRequest request;
    request.set_term(term);
    request.set_candidate_id(candidate_id);
    request.set_last_log_index(last_log_index);
    request.set_last_log_term(last_log_term);
    return request;
}

auto vosfs::raft::detail::MessageFactory::make_request_vote_response(
    std::span<char> resp_payload,
    uint64_t id,
    uint64_t term,
    bool vote_granted) -> rpc::RpcResult {
    RequestVoteResponse response;
    response.set_id(id);
    response.set_term(term);
    response.set_vote_granted(vote_granted);
    return rpc::serialize_response(response, resp_payload);
}

auto vosfs::raft::detail::MessageFactory::make_append_entries_request(
    uint64_t term,
    uint64_t leader_id,
    uint64_t prev_log_index,
    uint64_t prev_log_term,
    std::vector<LogEntry>& entries,
    uint64_t leader_commit) -> AppendEntriesRequest {
    AppendEntriesRequest request;
    request.set_term(term);
    request.set_leader_id(leader_id);
    request.set_prev_log_index(prev_log_index);
    request.set_prev_log_term(prev_log_term);
    for (auto& entry : entries) {
        request.mutable_entries()->Add(std::move(entry));
    }
    request.set_leader_commit(leader_commit);
    return request;
}

auto vosfs::raft::detail::MessageFactory::make_append_entries_response(
    std::span<char> resp_payload,
    uint64_t id,
    uint64_t term,
    bool success,
    uint64_t last_log_index,
    std::optional<uint64_t> conflict_index) -> rpc::RpcResult {
    AppendEntriesResponse response;
    response.set_id(id);
    response.set_term(term);
    response.set_success(success);
    response.set_last_log_index(last_log_index);
    if (conflict_index.has_value()) {
        response.set_conflict_index(conflict_index.value());
    }
    return rpc::serialize_response(response, resp_payload);
}

auto vosfs::raft::detail::MessageFactory::make_install_snapshot_request(
    uint64_t term,
    uint64_t leader_id,
    uint64_t last_included_index,
    uint64_t last_included_term,
    uint64_t offset,
    std::string&& data,
    bool done) -> InstallSnapshotRequest {
    InstallSnapshotRequest request;
    request.set_term(term);
    request.set_leader_id(leader_id);
    request.set_last_included_index(last_included_index);
    request.set_last_included_term(last_included_term);
    request.set_offset(offset);
    request.set_data(std::move(data));
    request.set_done(done);
    return request;
}

auto vosfs::raft::detail::MessageFactory::make_install_snapshot_response(
    std::span<char> resp_payload,
    uint64_t term) -> rpc::RpcResult {
    InstallSnapshotResponse response;
    response.set_term(term);
    return rpc::serialize_response(response, resp_payload);
}
