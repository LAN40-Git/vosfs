#pragma once
#include "vosfs/rpc/internal/config.hpp"
#include "vosfs/rpc/internal/protocol.hpp"
#include "vosfs/rpc/internal/invoker.hpp"
#include "vosfs/rpc/internal/session_manager.hpp"

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

private:
    explicit RpcProvider(uint16_t port, AuthMode auth_mode, kosio::net::TcpListener listener)
        : port_(port), auth_mode_(auth_mode), listener_(std::move(listener)) {}

public:
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
    void register_handler(
        ServiceType service_type,
        MethodType method_type,
        const RpcRequestHandler& handler);

public:
    [[REMEMBER_CO_AWAIT]]
    auto run() -> kosio::async::Task<Result<void>>;

    [[REMEMBER_CO_AWAIT]]
    auto shutdown() -> kosio::async::Task<Result<void>>;

private:
    auto handle_request(std::shared_ptr<detail::Session> session) -> kosio::async::Task<void>;

    auto send_response(std::shared_ptr<detail::Session> session) -> kosio::async::Task<void>;

private:
    using RpcRequest = detail::RpcInvoker::Request;

    uint16_t                port_;
    AuthMode                auth_mode_;
    kosio::net::TcpListener listener_;
    kosio::sync::Mutex      mutex_;
    bool                    is_shutdown_{true};
    std::atomic<bool>       is_listening_{false};
    detail::SessionManager  session_manager_;
    detail::RpcInvoker      invoker_;
};
} // namespace vosfs::rpc