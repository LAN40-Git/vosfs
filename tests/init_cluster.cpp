#include <kosio/common/debug.hpp>
#include "vosfs/raft/persister.hpp"

using namespace vosfs;
using namespace vosfs::raft;

auto main() -> int {
    auto has_persister = Persister::create("");
    if (!has_persister) {
        LOG_ERROR("{}", has_persister.error());
        return -1;
    }
    auto persister = std::move(has_persister.value());
    RaftNodeInfo raft_node_info;
    RaftClusterInfo raft_cluster_info;
    DataClusterInfo data_cluster_info;

    persister.init(raft_node_info, raft_cluster_info, data_cluster_info);
}