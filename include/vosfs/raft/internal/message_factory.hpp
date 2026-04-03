#pragma once
#include "vosfs/api/serverpb/raft.pb.h"

namespace vosfs::raft::detail {
class MessageFactory {
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
    static auto make_request_vote_response(uint64_t term, bool vote_granted) -> RequestVoteResponse;
};
} // namespace vosfs::raft::detail