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
    auto size = static_cast<int>(response.ByteSizeLong());
    if (size > resp_payload.size()) {
        return rpc::make_result(rpc::RpcResult::kMessageTooLarge);
    }

    if (!response.SerializeToArray(resp_payload.data(), size)) {
        return rpc::make_result(rpc::RpcResult::kMessageSerializeFailed);
    }
    return rpc::make_result(rpc::RpcResult::kSuccess, size);
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
    auto size = static_cast<int>(response.ByteSizeLong());
    if (size > resp_payload.size()) {
        return rpc::make_result(rpc::RpcResult::kMessageTooLarge);
    }

    if (!response.SerializeToArray(resp_payload.data(), size)) {
        return rpc::make_result(rpc::RpcResult::kMessageSerializeFailed);
    }
    return rpc::make_result(rpc::RpcResult::kSuccess, size);
}
