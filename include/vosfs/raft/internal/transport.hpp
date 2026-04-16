#pragma once
#include "vosfs/rpc/provider.hpp"
#include "vosfs/raft/internal/peer.hpp"
#include "vosfs/raft/persister.hpp"

namespace vosfs::raft::detail {
class Transport {
    using PeerMap = std::unordered_map<uint64_t, Peer>;
private:
    explicit Transport(
        RaftClusterInfo&& raft_cluster_info,
        RaftNodeInfo&& node_info,
        PeerMap&& peers)
        : raft_cluster_info_(std::move(raft_cluster_info))
        , raft_node_info_(node_info)
        , peers_(std::move(peers)) {}

public:
    static auto create(const Persister& persister) -> kosio::async::Task<Result<Transport>>;

public:
    auto shutdown() const -> kosio::async::Task<void>;

    auto apply_snapshot(Snapshot& snapshot) -> kosio::async::Task<void>;

public:
    [[REMEMBER_CO_AWAIT]]
    auto unicast_request(
        uint64_t peer_id,
        rpc::ServiceType service_type,
        rpc::MethodType method_type,
        std::string_view req_payload,
        const rpc::RpcCallback& callback) -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto unicast_request(
        uint64_t peer_id,
        rpc::ServiceType service_type,
        rpc::MethodType method_type,
        std::string&& req_payload,
        rpc::RpcCallback&& callback) -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto broadcast_request(
        rpc::ServiceType service_type,
        rpc::MethodType method_type,
        std::string_view req_payload,
        const rpc::RpcCallback& callback) -> kosio::async::Task<void>;

    // use kosio::spawn
    auto broadcast_request(
        rpc::ServiceType service_type,
        rpc::MethodType method_type,
        std::string&& req_payload,
        rpc::RpcCallback&& callback) -> kosio::async::Task<void>;

public:
    [[nodiscard]]
    auto cluster_id() const noexcept -> uint64_t { return raft_node_info_.id(); }
    [[nodiscard]]
    auto member_id() const noexcept -> uint64_t { return raft_node_info_.id(); }
    [[nodiscard]]
    auto name() const noexcept -> std::string_view { return raft_node_info_.name(); }
    [[nodiscard]]
    auto ip() const noexcept -> std::string_view { return raft_node_info_.ip(); }
    [[nodiscard]]
    auto peers() -> PeerMap& { return peers_; }
    [[nodiscard]]
    auto peer_count() const noexcept -> uint64_t { return peers_.size(); }

private:
    static auto build_cluster(const RaftClusterInfo& cluster_info, const RaftNodeInfo& node_info) -> kosio::async::Task<Result<PeerMap>>;

private:
    RaftClusterInfo raft_cluster_info_;
    RaftNodeInfo    raft_node_info_;
    PeerMap         peers_;
};
} // namespace vosfs::raft::detail