#pragma once
#include "vosfs/rpc/provider.hpp"
#include "vosfs/raft/internal/cluster.hpp"

namespace vosfs::raft {
class RaftNode;
} // namespace vosfs::raft

namespace vosfs::raft::detail {
class Transport {
private:
    explicit Transport(RaftCluster&& cluster)
        : cluster_(std::move(cluster)) {}

public:
    static auto create(RaftCluster&& cluster) -> kosio::async::Task<Result<Transport>>;

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

    auto broadcast_request(
        rpc::ServiceType service_type,
        rpc::MethodType method_type,
        std::string&& req_payload,
        rpc::RpcCallback&& callback) -> kosio::async::Task<void>;

public:
    [[nodiscard]]
    auto cluster_id() const noexcept -> uint64_t { return cluster_.cluster_id(); }
    [[nodiscard]]
    auto member_id() const noexcept -> uint64_t { return cluster_.member_id(); }
    [[nodiscard]]
    auto name() const noexcept -> std::string_view { return cluster_.name(); }
    [[nodiscard]]
    auto host() const noexcept -> std::string_view { return cluster_.host(); }
    [[nodiscard]]
    auto peer_count() const noexcept -> uint64_t { return cluster_.peer_count(); }

private:
    RaftCluster cluster_;
};
} // namespace vosfs::raft::detail