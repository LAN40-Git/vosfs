#pragma once
#include "vosfs/rpc/types.hpp"

namespace vosfs::rpc::detail {
class RpcInvoker {
    using Method = RpcRequestHandler;
    using MethodMap = std::unordered_map<ServiceType, std::unordered_map<MethodType, Method>>;

public:
    struct Request {
        uint64_t            request_id_{0};
        ServiceType         service_type_;
        MethodType          method_type_;
        RpcError::ErrorCode error_code_{RpcError::kSuccess};
    };
    using RequestQueue =

public:
    void register_method(ServiceType service_type, MethodType method_type, const Method& invoke);

    [[REMEMBER_CO_AWAIT]]
    auto invoke(
        ServiceType service_type,
        MethodType method_type,
        std::string_view req_payload,
        std::span<char> resp_payload) -> kosio::async::Task<InvokeResult>;

private:
    MethodMap methods_;
};
} // namespace vosfs::rpc::detail