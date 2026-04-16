#include <kosio/common/debug.hpp>
#include "vosfs/raft/persister.hpp"

using namespace vosfs;
using namespace vosfs::raft;

void add_raft_node_info(RaftClusterInfo& raft_cluster_info, uint64_t id, std::string&& name, std::string&& ip) {
    // 查询是否已经存在
    auto& raft_node_infos = raft_cluster_info.raft_node_infos();
    for (auto& raft_node_info : raft_node_infos) {
        if (raft_node_info.id() == id ||
            raft_node_info.name() == name ||
            raft_node_info.ip() == ip) {
            LOG_INFO("failed to add raft node info: raft node info exists");
            return;
        }
    }

    auto raft_node_info_ptr = raft_cluster_info.mutable_raft_node_infos()->Add();
    raft_node_info_ptr->set_id(id);
    raft_node_info_ptr->set_name(std::move(name));
    raft_node_info_ptr->set_ip(std::move(ip));
}

void add_data_node_info(DataClusterInfo& data_cluster_info, std::string&& ip, uint16_t port) {
    // 查询是否已经存在
    auto& data_node_infos = data_cluster_info.data_node_infos();
    for (auto& data_node_info : data_node_infos) {
        if (data_node_info.ip() == ip) {
            LOG_INFO("failed to add data node info: data node info exists");
            return;
        }
    }

    auto data_node_info_ptr = data_cluster_info.mutable_data_node_infos()->Add();
    data_node_info_ptr->set_ip(std::move(ip));
    data_node_info_ptr->set_port(port);
}

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

    raft_node_info.set_id(0);
    raft_node_info.set_name("MDS_1");
    raft_node_info.set_ip("172.20.179.151");

    add_raft_node_info(raft_cluster_info, 0, "MDS_1", "172.20.179.151");

    persister.init(raft_node_info, raft_cluster_info, data_cluster_info);
}