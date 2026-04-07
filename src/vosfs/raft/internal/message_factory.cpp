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
    uint64_t term,
    bool vote_granted) -> rpc::InvokeResult {
    RequestVoteResponse response;
    response.set_term(term);
    response.set_vote_granted(vote_granted);
    auto size = static_cast<int>(response.ByteSizeLong());
    if (!response.SerializeToArray(resp_payload.data(), size)) {
        return std::make_pair(rpc::RpcError::kMessageSerializeFailed, 0);
    }
    return std::make_pair(rpc::RpcError::kSuccess, size);
}
