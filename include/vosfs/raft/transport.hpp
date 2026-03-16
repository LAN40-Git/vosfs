#pragma once
#include "vosfs/rpc/provider.hpp"

namespace vosfs::raft::detail {
class Transport {
public:


private:
    rpc::RpcProvider raft_server_;
    rpc::RpcProvider client_server_;
};
} // namespace vosfs::raft::detail