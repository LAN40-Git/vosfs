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


    }
}

auto vosfs::raft::RaftNode::heartbeat_loop() -> kosio::async::Task<void> {

}

auto vosfs::raft::RaftNode::handle_request_vote_request(std::string_view req_payload) -> kosio::async::Task<void> {

}
