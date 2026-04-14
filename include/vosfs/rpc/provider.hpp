#pragma once
#include "vosfs/common/error.hpp"
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

private:
    explicit RpcProvider(uint16_t port, kosio::net::TcpListener listener)
        : port_(port), listener_(std::move(listener)) {}

public:
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
    void register_handler(
        ServiceType service_type,
        MethodType method_type,
        const RpcRequestHandler& handler);

public:
    /// @brief Let provider run
    /// @note not-thread-safe!
    auto run() -> kosio::async::Task<void>;

    /// @brief Let provider shutdown
    /// @note not-thread-safe!
    [[REMEMBER_CO_AWAIT]]
    auto shutdown() -> kosio::async::Task<void>;

private:
    auto handle_request(std::shared_ptr<detail::Session> session) -> kosio::async::Task<void>;

    auto send_response(std::shared_ptr<detail::Session> session) -> kosio::async::Task<void>;

private:
    using RpcRequest = detail::RpcInvoker::Request;

    uint16_t                port_;
    kosio::net::TcpListener listener_;
    std::atomic<bool>       is_shutdown_{true};
    detail::SessionManager  session_manager_;
    detail::RpcInvoker      invoker_;
};
} // namespace vosfs::rpc