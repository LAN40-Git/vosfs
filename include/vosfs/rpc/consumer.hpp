#pragma once
#include <utility>

#include "vosfs/rpc/util.hpp"

namespace vosfs::rpc {
using RpcCallback = std::function<kosio::async::Task<void>(std::string_view resp_payload)>;
class RpcConsumer {
    using RpcCallbackMap = tbb::concurrent_hash_map<uint64_t, RpcCallback>;

    enum Status {
        Running = 0,
        ShuttingDown,
        ShutDown
    };
public:
    explicit RpcConsumer(std::string_view server_host, uint16_t server_port, kosio::net::TcpStream stream)
        : server_host_(server_host), server_port_(server_port), stream_(std::move(stream)) {
        callbacks_.rehash(4096);
    }

public:
    // Delete copy
    RpcConsumer(const RpcConsumer&) = delete;
    auto operator=(const RpcConsumer&) -> RpcConsumer& = delete;

    // Delete move
    RpcConsumer(RpcConsumer&&) = delete;
    auto operator=(RpcConsumer&&) -> RpcConsumer& = delete;

public:
    [[REMEMBER_CO_AWAIT]]
    static auto create(std::string_view server_host, uint16_t server_port) -> kosio::async::Task<Result<std::unique_ptr<RpcConsumer>>>;

public:
    [[REMEMBER_CO_AWAIT]]
    auto send_request(
        ServiceType service_type,
        MethodType method_type,
        std::string_view req_payload,
        const RpcCallback& callback) -> kosio::async::Task<Result<void>>;
    [[REMEMBER_CO_AWAIT]]
    auto send_request(
        ServiceType service_type,
        MethodType method_type,
        std::string_view req_payload,
        RpcCallback&& callback) -> kosio::async::Task<Result<void>>;
    [[REMEMBER_CO_AWAIT]]
    auto run() -> kosio::async::Task<Result<void>>;
    [[REMEMBER_CO_AWAIT]]
    auto shutdown() -> kosio::async::Task<Result<void>>;

private:
    [[REMEMBER_CO_AWAIT]]
    auto trigger_callback(uint64_t request_id, std::string_view resp_payload) -> kosio::async::Task<void>;
    [[REMEMBER_CO_AWAIT]]
    auto handle_response() -> kosio::async::Task<void>;
    [[REMEMBER_CO_AWAIT]]
    auto send_shutdown_request() -> kosio::async::Task<Result<void>>;

private:
    std::string           server_host_;
    uint16_t              server_port_;
    kosio::net::TcpStream stream_;
    uint64_t              request_id_{0};
    RpcCallbackMap        callbacks_;
    kosio::sync::Mutex    mutex_;
    Status                status_{Running};

};
} // namespace vosfs::rpc