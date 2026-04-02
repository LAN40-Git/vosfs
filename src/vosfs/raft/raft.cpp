#include "vosfs/raft/raft.hpp"

auto vosfs::raft::RaftNode::start_election_timeout() -> kosio::async::Task<void> {
    kosio::util::FastRand rand;
    while (!is_shutdown_.load(std::memory_order_relaxed)) {
        // 150-300ms election timeout
        auto election_timeout = rand.rand_range(150, 300);
        co_await kosio::time::sleep(election_timeout);


    }
}
