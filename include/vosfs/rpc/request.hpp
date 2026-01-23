#pragma once
#include "vosfs/rpc/util.hpp"
#include <functional>

namespace vosfs::rpc {
using RpcCallback = std::function<kosio::async::Task<>(std::string_view resp_payload)>;

namespace detail {
class RpcRequest {
public:
    RpcRequest() = default;
    explicit RpcRequest(ServiceType service_type, MethodType method_type, std::string_view req_payload, const RpcCallback& callback)
        : service_type_(service_type)
        , method_type_(method_type)
        , req_payload_(std::string{req_payload})
        , callback_(callback) {}

    explicit RpcRequest(ServiceType service_type, MethodType method_type, std::string_view req_payload, RpcCallback&& callback)
        : service_type_(service_type)
        , method_type_(method_type)
        , req_payload_(std::string{req_payload})
        , callback_(std::move(callback)) {}

    explicit RpcRequest(ServiceType service_type, MethodType method_type, std::string&& req_payload, RpcCallback&& callback)
        : service_type_(service_type)
        , method_type_(method_type)
        , req_payload_(std::move(req_payload))
        , callback_(std::move(callback)) {}

    // Delete copy
    RpcRequest(const RpcRequest&) = delete;
    auto operator=(const RpcRequest&) = delete;

    RpcRequest(RpcRequest&& other) noexcept
        : service_type_(other.service_type_)
        , method_type_(other.method_type_)
        , req_payload_(std::move(other.req_payload_))
        , callback_(std::move(other.callback_)) {}

    auto operator=(RpcRequest&& other) noexcept -> RpcRequest& {
        service_type_ = other.service_type_;
        method_type_ = other.method_type_;
        req_payload_ = std::move(other.req_payload_);
        callback_ = std::move(other.callback_);
        return *this;
    }

private:
    ServiceType service_type_{};
    MethodType  method_type_{};
    std::string req_payload_{};
    RpcCallback callback_;
};
} // namespace detail
} // namespace vosfs::rpc