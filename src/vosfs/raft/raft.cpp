#include "vosfs/raft/raft.hpp"

auto vosfs::raft::RaftNode::election_loop() -> kosio::async::Task<void> {
    kosio::util::FastRand rand;
    while (!is_shutdown_.load(std::memory_order_relaxed)) {
        // [150,300]ms timeout
        auto timeout = rand.rand_range(150, 300);
        co_await kosio::time::sleep(timeout);

        if (kosio::util::current_ms() - last_reset_time_.load(std::memory_order_relaxed) < timeout ||
            role_.load(std::memory_order_acquire) == kLeader) {
            continue;
        }

        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);

        // Check again
        if (role_.load(std::memory_order_relaxed) == kLeader) {
            continue;
        }

        do_election();
    }
}

auto vosfs::raft::RaftNode::heartbeat_loop() -> kosio::async::Task<void> {

}

void vosfs::raft::RaftNode::do_election() {
    role_.store(kCandidate, std::memory_order_relaxed);
    current_term_.fetch_add(1, std::memory_order_relaxed);
    // votes for myself at this term
    voted_for_ = transport_.member_id();
    votes_ = 1;
    // broadcast reqeust vote request
    auto request = detail::MessageFactory::make_request_vote_request(
        current_term_.load(std::memory_order_relaxed),
        transport_.member_id(),
        logs_.last_log_index(),
        logs_.last_log_term());
    kosio::spawn(transport_.broadcast_request(
        rpc::ServiceType::kRaft,
        rpc::MethodType::kRaftRequestVote,
        request.SerializeAsString(),
        [this](std::string_view resp_payload) -> kosio::async::Task<void> {
            co_await this->handle_request_vote_response(resp_payload);
        }));
}

void vosfs::raft::RaftNode::do_heartbeat() {

}

void vosfs::raft::RaftNode::increase_term_to(uint64_t term) {
    votes_ = 0;
    voted_for_.reset();
    current_term_.store(term, std::memory_order_release);
}

auto vosfs::raft::RaftNode::handle_request_vote_request(
    std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<rpc::InvokeResult> {
    RequestVoteRequest request;
    if (!request.ParseFromArray(req_payload.data(), req_payload.size())) {
        co_return std::make_pair(rpc::RpcError::kMessageParseFailed, 0);
    }

    auto term = request.term();
    auto candidate_id = request.candidate_id();
    auto last_log_index = request.last_log_index();
    auto last_log_term = request.last_log_term();

    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    auto current_term = current_term_.load(std::memory_order_relaxed);
    if (term < current_term) {
        co_return detail::MessageFactory::make_request_vote_response(resp_payload, current_term, false);
    }

    if (term > current_term) {
        increase_term_to(term);
        role_.store(kFollower, std::memory_order_release);
        last_reset_time_.store(kosio::util::current_ms(), std::memory_order_relaxed);
        // TODO: persist

    }

    bool up_to_date_log{false};

    if (last_log_index > logs_.last_log_index() ||
        (last_log_index == logs_.last_log_index() &&
            last_log_term == logs_.last_log_term())) {
        up_to_date_log = true;
    }

    bool can_vote = !voted_for_.has_value() && up_to_date_log;

    if (can_vote) {
        voted_for_ = candidate_id;
    }

    co_return detail::MessageFactory::make_request_vote_response(resp_payload, term, can_vote);
}

auto vosfs::raft::RaftNode::handle_request_vote_response(std::string_view resp_payload) -> kosio::async::Task<void> {
    RequestVoteResponse response;
    if (!response.ParseFromArray(resp_payload.data(), resp_payload.size())) {
        co_return;
    }

    auto term = response.term();
    auto vote_granted = response.vote_granted();

    if (term < current_term_.load(std::memory_order_acquire)) {
        co_return;
    }

    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    auto current_term = current_term_.load(std::memory_order_relaxed);
    if (term < current_term) {
        co_return;
    }


}
