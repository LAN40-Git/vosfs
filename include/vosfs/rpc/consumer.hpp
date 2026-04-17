#pragma once
#include <utility>
#include <kosio/net.hpp>
#include <kosio/sync.hpp>
#include <tbb/concurrent_hash_map.h>
#include "vosfs/common/error.hpp"
#include "vosfs/rpc/internal/config.hpp"
#include "vosfs/rpc/internal/protocol.hpp"

namespace vosfs::rpc {
using RpcCallback = std::function<kosio::async::Task<void>(std::string_view resp_payload)>;
class RpcConsumer {
    using RpcCallbackMap = std::unordered_map<uint64_t, RpcCallback>;

private:
    explicit RpcConsumer(std::string_view server_ip, uint16_t server_port)
        : server_ip_(server_ip)
        , server_port_(server_port)
        , stream_(kosio::net::TcpStream{kosio::net::detail::Socket{-1}}) { callbacks_.rehash(4096); }

public:
    // Delete copy
    RpcConsumer(const RpcConsumer&) = delete;
    auto operator=(const RpcConsumer&) -> RpcConsumer& = delete;

    // Delete move
    RpcConsumer(RpcConsumer&&) = delete;
    auto operator=(RpcConsumer&&) -> RpcConsumer& = delete;

public:
    [[REMEMBER_CO_AWAIT]]
    static auto create(std::string_view server_ip, uint16_t server_port) -> kosio::async::Task<kosio::Result<std::unique_ptr<RpcConsumer>>>;

public:
    [[REMEMBER_CO_AWAIT]]
    auto run() -> kosio::async::Task<void>;
    [[REMEMBER_CO_AWAIT]]
    auto shutdown() const -> kosio::async::Task<void>;
    auto is_shutdown() const -> bool;

public:
    [[REMEMBER_CO_AWAIT]]
    auto send_request(
        ServiceType service_type,
        MethodType method_type,
        std::string_view req_payload,
        const RpcCallback& callback) -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto send_request(
        ServiceType service_type,
        MethodType method_type,
        std::string&& req_payload,
        RpcCallback&& callback) -> kosio::async::Task<void>;

private:
    [[REMEMBER_CO_AWAIT]]
    auto remove_callback(uint64_t request_id) -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto trigger_callback(uint64_t request_id, std::string_view resp_payload) -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_response() -> kosio::async::Task<void>;

private:

    std::string           server_ip_;
    uint16_t              server_port_;
    kosio::net::TcpStream stream_;
    uint64_t              request_id_{0}; // current request id
    std::atomic<bool>     is_shutdown_{true};
    RpcCallbackMap        callbacks_;
    kosio::sync::Mutex    mutex_;
};

using RpcClient = std::unique_ptr<RpcConsumer>;
} // namespace vosfs::rpc