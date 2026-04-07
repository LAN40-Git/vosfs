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
        uint64_t term,
        bool vote_granted) -> rpc::InvokeResult;
};
} // namespace vosfs::raft::detail