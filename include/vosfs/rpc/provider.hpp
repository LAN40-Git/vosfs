#pragma once
#include "vosfs/rpc/session_manager.hpp"

namespace vosfs::rpc {
class RpcProvider {
public:
    explicit RpcProvider(uint16_t port, kosio::net::TcpListener listener)
        : port_(port), listener_(std::move(listener)) {}

    // Delete copy
    RpcProvider(const RpcProvider&) = delete;
    auto operator=(const RpcProvider&) -> RpcProvider& = delete;

    // Delete move
    RpcProvider(RpcProvider&&) = delete;
    auto operator=(RpcProvider&&) -> RpcProvider&  = delete;

public:
    [[REMEMBER_CO_AWAIT]]
    static auto create(uint16_t port) -> kosio::async::Task<Result<std::unique_ptr<RpcProvider>>>;

public:
    void register_invoke(
        ServiceType service_type,
        MethodType method_type,
        const detail::Invoke& invoke);

private:
    [[REMEMBER_CO_AWAIT]]
    auto run() -> kosio::async::Task<Result<void>>;

    [[REMEMBER_CO_AWAIT]]
    auto shutdown() -> kosio::async::Task<Result<void>>;

    auto handle_request(std::shared_ptr<detail::Session> session) -> kosio::async::Task<void>;

    auto send_response(std::shared_ptr<detail::Session> session) -> kosio::async::Task<void>;

private:
    using InvokeMap = std::unordered_map<ServiceType, std::unordered_map<MethodType, detail::Invoke>>;

    uint16_t                port_;
    kosio::net::TcpListener listener_;
    bool                    is_shutdown_{true};
    kosio::sync::Mutex      mutex_;
    InvokeMap               invokes_;
    detail::SessionManager  session_manager_;
};
} // namespace vosfs::rpc