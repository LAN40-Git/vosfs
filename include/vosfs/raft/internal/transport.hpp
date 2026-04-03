#pragma once
#include "vosfs/rpc/provider.hpp"
#include "vosfs/raft/internal/cluster.hpp"

namespace vosfs::raft {
class RaftNode;
} // namespace vosfs::raft

namespace vosfs::raft::detail {
class Transport {
private:
    explicit Transport(
        RaftCluster&& cluster,
        std::unique_ptr<rpc::RpcProvider> raft_provider,
        std::unique_ptr<rpc::RpcProvider> client_provider);

public:
    static auto create(RaftCluster&& cluster) -> kosio::async::Task<Result<Transport>>;

public:
    void run() const;

    [[REMEMBER_CO_AWAIT]]
    auto shutdown() const -> kosio::async::Task<Result<void>>;

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

private:
    RaftCluster                       cluster_;
    std::unique_ptr<rpc::RpcProvider> raft_provider_;
    std::unique_ptr<rpc::RpcProvider> client_provider_;
};
} // namespace vosfs::raft::detail