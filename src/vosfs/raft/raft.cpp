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
    while (!is_shutdown_.load(std::memory_order_relaxed)) {
        co_await kosio::time::sleep(detail::HEARTBEAT_INTERVAL);

        if (role_.load(std::memory_order_acquire) != kLeader) {
            continue;
        }

        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);

        if (role_.load(std::memory_order_relaxed) != kLeader) {
            continue;
        }

        do_heartbeat();
    }
}

void vosfs::raft::RaftNode::persist_current_term() const {
    auto ret = persister_.persist(detail::CURRENT_TERM_KEY,
        std::to_string(current_term_.load(std::memory_order_relaxed)));
    if (!ret) {
        LOG_FATAL("{}", ret.error());
        std::abort();
    }
}

void vosfs::raft::RaftNode::persist_voted_for() const {
    if (voted_for_.has_value()) {
        auto ret = persister_.persist(detail::CURRENT_TERM_KEY,
        std::to_string(voted_for_.value()));
        if (!ret) {
            LOG_FATAL("{}", ret.error());
            std::abort();
        }
    } else {
        auto ret = persister_.persist(detail::CURRENT_TERM_KEY,
        "");
        if (!ret) {
            LOG_FATAL("{}", ret.error());
            std::abort();
        }
    }
}

void vosfs::raft::RaftNode::do_election() {
    role_.store(kCandidate, std::memory_order_relaxed);
    votes_ = 1;
    // votes for myself at this term
    voted_for_ = transport_.member_id();
    persist_voted_for();
    current_term_.fetch_add(1, std::memory_order_relaxed);
    persist_current_term();
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
    auto current_term = current_term_.load(std::memory_order_relaxed);
    auto leader_id  = transport_.member_id();
    auto commit_index = commit_index_;
    auto last_log_index = logs_.last_log_index();
    for (const auto& [member_id, next_index] : next_index_) {
        std::vector<LogEntry> entries;
        if (next_index <= last_log_index) {
            if (next_index < logs_.start_index()) {
                // TODO: send snapshot
                continue;
            } else {
                auto batch_size = std::min(detail::MAX_ENTRIES_PER_APPEND, last_log_index - next_index + 1);
                entries = logs_.get_entries(next_index, batch_size);
            }
        }

        auto prev_log_index = next_index - 1;
        uint64_t prev_log_term  = logs_.get_term(prev_log_index);

        auto request = detail::MessageFactory::make_append_entries_request(
            current_term_.load(std::memory_order_relaxed),
            transport_.member_id(),
            prev_log_index,
            prev_log_term,
            entries,
            commit_index_);

        kosio::spawn(transport_.unicast_request(
            member_id,
            rpc::ServiceType::kRaft,
            rpc::MethodType::kRaftAppendEntries,
            request.SerializeAsString(),
            [this](std::string_view resp_payload) -> kosio::async::Task<void> {
                co_await this->handle_append_entries_response(resp_payload);
            }));
    }
}

void vosfs::raft::RaftNode::increase_term_to(uint64_t term) {
    votes_ = 0;
    voted_for_.reset();
    persist_voted_for();
    current_term_.store(term, std::memory_order_release);
    persist_current_term();
    role_.store(kFollower, std::memory_order_release);
    last_reset_time_.store(kosio::util::current_ms(), std::memory_order_relaxed);
}

void vosfs::raft::RaftNode::become_leader() {
    votes_ = 0;
    voted_for_.reset();
    persist_voted_for();
    role_.store(kLeader, std::memory_order_release);
    // update next_index
    auto& peers = transport_.peers();
    for (const auto& peer : peers | std::views::values) {
        next_index_[peer.member_id()] = logs_.last_log_index() + 1;
    }
}

auto vosfs::raft::RaftNode::handle_request_vote_request(
    std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
    RequestVoteRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
        co_return rpc::make_result(rpc::RpcResult::kMessageParseFailed);
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

auto vosfs::raft::RaftNode::handle_append_entries_request(
    std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {

}

auto vosfs::raft::RaftNode::handle_request_vote_response(std::string_view resp_payload) -> kosio::async::Task<void> {
    RequestVoteResponse response;
    if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
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

    if (term > current_term) {
        increase_term_to(term);
        co_return;
    }

    if (role_.load(std::memory_order_relaxed) == kLeader) {
        co_return;
    }

    if (vote_granted) {
        if (++votes_ > transport_.peer_count() / 2 &&
            role_.load(std::memory_order_relaxed) == kCandidate) {
            become_leader();
        }
    }
}
