#pragma once
#include "vosfs/rpc/provider.hpp"

namespace vosfs::auth {
class AuthServer {
public:

private:
    rpc::RpcProvider rpc_server_;
};
} // namespace vosfs::auth