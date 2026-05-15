#include "vosfs/storage/node.hpp"
using namespace vosfs;
using namespace kosio;
using namespace kosio::async;

auto main_coro() -> Task<void> {
    auto data_node = storage::DataNode{3, "0.0.0.0", 7072, "node3"};
    co_await data_node.wait();
}

auto main() -> int {
    runtime::MultiThreadBuilder::default_create().block_on(main_coro());
}
