#pragma once
#include "vosfs/rpc/provider.hpp"
#include "vosfs/raft/internal/peer.hpp"
#include "vosfs/raft/persister.hpp"

namespace vosfs::raft::detail {
class Transport {
    using PeerMap = std::unordered_map<uint64_t, Peer>;
private:
    explicit Transport(
        ClusterInfo&& cluster_info,
        NodeInfo&& node_info,
        PeerMap&& peers)
        : cluster_info_(std::move(cluster_info))
        , node_info_(node_info)
        , peers_(std::move(peers)) {}

public:
    static auto create(const Persister& persister) -> kosio::async::Task<Result<Transport>>;

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
    auto cluster_id() const noexcept -> uint64_t { return cluster_info_.id(); }
    [[nodiscard]]
    auto member_id() const noexcept -> uint64_t { return node_info_.id(); }
    [[nodiscard]]
    auto name() const noexcept -> std::string_view { return node_info_.name(); }
    [[nodiscard]]
    auto host() const noexcept -> std::string_view { return node_info_.host(); }
    [[nodiscard]]
    auto peers() -> PeerMap& { return peers_; }
    [[nodiscard]]
    auto peer_count() const noexcept -> uint64_t { return peers_.size(); }

private:
    ClusterInfo cluster_info_;
    NodeInfo    node_info_;
    PeerMap     peers_;
};
} // namespace vosfs::raft::detail