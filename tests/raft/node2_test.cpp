#include "vosfs/raft/node.hpp"
using namespace vosfs;
using namespace kosio;
using namespace kosio::async;

auto main_coro() -> Task<void> {
    auto config = raft::ConfigBuilder::options()
        .set_data_dir("./node2")
        .set_cluster_id(1)
        .set_member_id(2)
        .set_name("node2")
        .set_host("127.0.0.1")
        .set_port(8081)
        .add_node(1, "node1", "127.0.0.1", 8080)
        .add_node(2, "node2", "127.0.0.1", 8081)
        .build();
    config.to_json("node2.json");

    auto has_raft_node = raft::RaftNode::create("node2.json");
    if (!has_raft_node) {
        LOG_ERROR("{}", has_raft_node.error());
        co_return;
    }

    auto raft_node = std::move(has_raft_node.value());
    co_await raft_node->wait();
}

auto main() -> int {
    runtime::MultiThreadBuilder::default_create().block_on(main_coro());
}
