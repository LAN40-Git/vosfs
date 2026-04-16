#include "vosfs/raft/internal/transport.hpp"
#include <ranges>

auto vosfs::raft::detail::Transport::create(const Persister& persister) -> kosio::async::Task<Result<Transport>> {
    // 恢复节点信息
    auto has_node_info = persister.load_raft_node_info();
    if (!has_node_info) {
        co_return std::unexpected{has_node_info.error()};
    }
    auto node_info = std::move(has_node_info.value());

    // 恢复集群信息
    auto has_cluster_info = persister.load_raft_cluster_info();
    if (!has_cluster_info) {
        co_return std::unexpected{has_cluster_info.error()};
    }
    auto cluster_info = std::move(has_cluster_info.value());
    auto has_peers = co_await build_cluster(cluster_info, node_info);
    if (!has_peers) {
        co_return std::unexpected{has_peers.error()};
    }
    auto peers = std::move(has_peers.value());

    co_return Transport{std::move(cluster_info), std::move(node_info), std::move(peers)};
}

auto vosfs::raft::detail::Transport::shutdown() const -> kosio::async::Task<void> {
    for (const auto& peer : peers_ | std::views::values) {
        co_await peer.shutdown();
    }
}

auto vosfs::raft::detail::Transport::apply_snapshot(Snapshot& snapshot) -> kosio::async::Task<void> {
    co_await shutdown();
    peers_.clear();
    auto& raft_node_infos = snapshot.raft_node_infos();
    raft_cluster_info_.mutable_raft_node_infos()->Clear();
    while (!raft_node_infos.empty()) {
        auto* node_info = snapshot.mutable_raft_node_infos()->ReleaseLast();
        raft_cluster_info_.mutable_raft_node_infos()->AddAllocated(node_info);
    }

    auto ret = co_await build_cluster(raft_cluster_info_, raft_node_info_);
    if (!ret) {
        LOG_FATAL("{}", ret.error());
        std::abort();
    }
    peers_ = std::move(ret.value());
}

auto vosfs::raft::detail::Transport::unicast_request(
    uint64_t peer_id, rpc::ServiceType service_type,
    rpc::MethodType method_type, std::string_view req_payload, const rpc::RpcCallback& callback)
    -> kosio::async::Task<void> {
    auto it = peers_.find(peer_id);
    if (it == peers_.end()) {
        LOG_ERROR("peer {} not found", peer_id);
        co_return;
    }

    auto& peer = it->second;
    co_await peer.send_request(service_type, method_type, req_payload, callback);
}

auto vosfs::raft::detail::Transport::unicast_request(uint64_t peer_id, rpc::ServiceType service_type,
    rpc::MethodType method_type, std::string&& req_payload, rpc::RpcCallback&& callback)
    -> kosio::async::Task<void> {
    auto it = peers_.find(peer_id);
    if (it == peers_.end()) {
        LOG_ERROR("peer {} not found", peer_id);
        co_return;
    }

    auto& peer = it->second;
    co_await peer.send_request(service_type, method_type, std::move(req_payload), callback);
}

auto vosfs::raft::detail::Transport::broadcast_request(rpc::ServiceType service_type, rpc::MethodType method_type,
    std::string_view req_payload, const rpc::RpcCallback& callback)
    -> kosio::async::Task<void> {
    for (auto& peer : peers_ | std::views::values) {
        co_await peer.send_request(service_type, method_type, req_payload, callback);
    }
}

auto vosfs::raft::detail::Transport::broadcast_request(
    rpc::ServiceType service_type, rpc::MethodType method_type,
    std::string&& req_payload, rpc::RpcCallback&& callback)
    -> kosio::async::Task<void> {
    for (auto& peer : peers_ | std::views::values) {
        co_await peer.send_request(service_type, method_type, std::move(req_payload), callback);
    }
}

auto vosfs::raft::detail::Transport::build_cluster(const RaftClusterInfo& raft_cluster_info, const RaftNodeInfo& raft_node_info) -> kosio::async::Task<Result<PeerMap>> {
    auto& node_infos = raft_cluster_info.raft_node_infos();
    PeerMap peers;
    for (auto& info : node_infos) {
        if (info.id() == raft_node_info.id()) {
            continue;
        }

        auto ret = co_await Peer::create(info);
        if (!ret) {
            co_return std::unexpected{ret.error()};
        }
        auto peer = std::move(ret.value());
        peers.emplace(peer.member_id(), std::move(ret.value()));
    }
    co_return peers;
}

