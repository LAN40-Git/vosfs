#include "vosfs/rpc/internal/invoker.hpp"

void vosfs::rpc::detail::RpcInvoker::register_method(ServiceType service_type, MethodType method_type, const Method& invoke) {
    methods_[service_type][method_type] = invoke;
}

auto vosfs::rpc::detail::RpcInvoker::invoke(
    ServiceType service_type,
    MethodType method_type,
    std::string_view req_payload,
    std::span<char> resp_payload) -> kosio::async::Task<InvokeResult> {
    // find method
    auto it_service = methods_.find(service_type);
    if (it_service == methods_.end()) {
        co_return std::make_pair(RpcError::kFindServiceTypeFailed, 0);
    }
    auto it_method = it_service->second.find(method_type);
    if (it_method == it_service->second.end()) {
        co_return std::make_pair(RpcError::kFindMethodTypeFailed, 0);
    }
    auto& method = it_method->second;

    co_return co_await method(req_payload, resp_payload);
}

auto vosfs::rpc::detail::RpcInvoker::is_unauth_method(ServiceType service_type, MethodType method_type) const noexcept -> bool {
    if ((service_type != ServiceType::kAuth && service_type != ServiceType::kConn) ||
        method_type == MethodType::kAuthSignout) {
        return false;
    }
    return true;
}
