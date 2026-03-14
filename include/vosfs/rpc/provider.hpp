#pragma once
#include "vosfs/rpc/session_manager.hpp"

namespace vosfs::rpc {
class RpcProvider {
    enum Status {
        Running = 0,
        ShuttingDown,
        ShutDown
    };

public:
    enum class AuthMode {
        NONE = 0,
        REQUIRED
    };

public:
    explicit RpcProvider(uint16_t port, AuthMode auth_mode, kosio::net::TcpListener listener)
        : port_(port), auth_mode_(auth_mode), listener_(std::move(listener)) {}

    // Delete copy
    RpcProvider(const RpcProvider&) = delete;
    auto operator=(const RpcProvider&) -> RpcProvider& = delete;

    // Delete move
    RpcProvider(RpcProvider&&) = delete;
    auto operator=(RpcProvider&&) -> RpcProvider&  = delete;

public:
    [[REMEMBER_CO_AWAIT]]
    static auto create(uint16_t port, AuthMode auth_mode) -> kosio::async::Task<Result<std::unique_ptr<RpcProvider>>>;

public:
    void register_invoke(
        ServiceType service_type,
        MethodType method_type,
        const detail::Invoke& invoke);

public:
    auto run() -> kosio::async::Task<Result<void>>;

    [[REMEMBER_CO_AWAIT]]
    auto shutdown() -> kosio::async::Task<Result<void>>;

private:
    auto handle_request(std::shared_ptr<detail::Session> session) -> kosio::async::Task<void>;

    auto send_response(std::shared_ptr<detail::Session> session) -> kosio::async::Task<void>;

private:
    using InvokeMap = std::unordered_map<ServiceType, std::unordered_map<MethodType, detail::Invoke>>;

    uint16_t                port_;
    AuthMode                auth_mode_;
    kosio::net::TcpListener listener_;
    InvokeMap               invokes_;
    kosio::sync::Mutex      mutex_;
    bool                    is_shutdown_{true};
    std::atomic<bool>       is_listening_{false};
    detail::SessionManager  session_manager_;
};
} // namespace vosfs::rpc