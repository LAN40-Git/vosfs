#pragma once
#include "vosfs/rpc/types.hpp"

namespace vosfs::rpc::detail {
struct FixedRpcRequestHeader {
    uint64_t    request_id{0};   // 8 bytes
    ServiceType service_type{0}; // 1 byte
    MethodType  method_type{0};  // 1 byte
    uint32_t    payload_size{0}; // 4 bytes
    // 14 bytes total.
};

struct FixedRpcResponseHeader {
    uint64_t          request_id{0};   // 8 bytes
    uint32_t          payload_size{0}; // 4 bytes
    RpcResult::Status status{0};       // 1 byte
    // 13 bytes total
};
} // namespace vosfs::rpc::detail