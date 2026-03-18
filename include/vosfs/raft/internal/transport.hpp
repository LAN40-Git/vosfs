#pragma once
#include "vosfs/rpc/provider.hpp"

namespace vosfs::raft::detail {
class Transport {
public:
    explicit Transport(
        std::unique_ptr<rpc::RpcProvider> raft_server,
        std::unique_ptr<rpc::RpcProvider> client_server)
        : raft_server_(std::move(raft_server)), client_server_(std::move(client_server)) {}

private:
    std::unique_ptr<rpc::RpcProvider> raft_server_;
    std::unique_ptr<rpc::RpcProvider> client_server_;
};
} // namespace vosfs::raft::detail