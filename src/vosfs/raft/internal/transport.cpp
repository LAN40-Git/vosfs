#include "vosfs/raft/internal/transport.hpp"
#include <ranges>

auto vosfs::raft::detail::Transport::create(ClusterInfo&& cluster_info, NodeInfo&& node_info) -> kosio::async::Task<Result<Transport>> {
    auto node_infos = cluster_info.node_infos();

    PeerMap peers;
    for (auto& info : node_infos) {
        if (info.id() == node_info.id()) {
            continue;
        }

        auto ret = co_await Peer::create(std::move(info));
        if (!ret) {
            co_return std::unexpected{ret.error()};
        }
        auto peer = std::move(ret.value());
        peers.emplace(peer.member_id(), std::move(ret.value()));
    }

    co_return Transport{std::move(cluster_info), std::move(node_info), std::move(peers)};
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
    if (auto ret = co_await peer.check_status(); !ret) {
        LOG_ERROR("{}", ret.error());
        co_return;
    }

    if (auto ret = co_await peer.send_request(service_type, method_type, req_payload, callback)) {
        LOG_ERROR("{}", ret.error());
        co_return;
    }
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
    if (auto ret = co_await peer.check_status(); !ret) {
        LOG_ERROR("{}", ret.error());
        co_return;
    }

    if (auto ret = co_await peer.send_request(service_type, method_type, std::move(req_payload), callback)) {
        LOG_ERROR("{}", ret.error());
    }
}

auto vosfs::raft::detail::Transport::broadcast_request(rpc::ServiceType service_type, rpc::MethodType method_type,
    std::string_view req_payload, const rpc::RpcCallback& callback)
    -> kosio::async::Task<void> {
    for (auto& peer : peers_ | std::views::values) {
        if (auto ret = co_await peer.check_status(); !ret) {
            LOG_ERROR("{}", ret.error());
            continue;
        }

        if (auto ret = co_await peer.send_request(service_type, method_type, req_payload, callback)) {
            LOG_ERROR("{}", ret.error());
        }
    }
}

auto vosfs::raft::detail::Transport::broadcast_request(
    rpc::ServiceType service_type, rpc::MethodType method_type,
    std::string&& req_payload, rpc::RpcCallback&& callback)
    -> kosio::async::Task<void> {
    for (auto& peer : peers_ | std::views::values) {
        if (auto ret = co_await peer.check_status(); !ret) {
            LOG_ERROR("{}", ret.error());
            continue;
        }

        if (auto ret = co_await peer.send_request(service_type, method_type, std::move(req_payload), callback)) {
            LOG_ERROR("{}", ret.error());
        }
    }
}

