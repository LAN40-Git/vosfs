#pragma once
#include "vosfs/rpc/provider.hpp"
#include "vosfs/raft/internal/cluster.hpp"

namespace vosfs::raft {
class RaftNode;
} // namespace vosfs::raft

namespace vosfs::raft::detail {
class Transport {
    friend class RaftNode;
private:
    explicit Transport(
        RaftCluster&& cluster,
        std::unique_ptr<rpc::RpcProvider> raft_provider,
        std::unique_ptr<rpc::RpcProvider> client_provider);

public:
    static auto create(RaftCluster&& cluster) -> kosio::async::Task<Result<Transport>>;

public:
    void run();

private:
    RaftCluster                       cluster_;
    std::unique_ptr<rpc::RpcProvider> raft_provider_;
    std::unique_ptr<rpc::RpcProvider> client_provider_;
};
} // namespace vosfs::raft::detail