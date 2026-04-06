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
    uint64_t term,
    bool vote_granted) -> RequestVoteResponse {

}
