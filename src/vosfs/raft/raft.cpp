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
    //kosio::spawn(transport_)
}

auto vosfs::raft::RaftNode::handle_request_vote_request(std::string_view req_payload) -> kosio::async::Task<void> {

}
