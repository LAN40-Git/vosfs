#pragma once
#include "vosfs/rpc/util.hpp"

namespace vosfs::rpc {
using RpcCallback = std::function<kosio::async::Task<void>(std::string_view resp_payload)>;
class RpcConsumer {
    using RpcCallbackMap = tbb::concurrent_hash_map<uint64_t, RpcCallback>;

public:
    explicit RpcConsumer(kosio::net::TcpStream stream)
        : stream_(std::move(stream)) {
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
    static auto create(std::string_view host, uint16_t port) -> kosio::async::Task<Result<std::unique_ptr<RpcConsumer>>>;

public:
    [[REMEMBER_CO_AWAIT]]
    auto send_request(
        detail::ServiceType service_type,
        detail::MethodType method_type,
        std::string_view req_payload,
        const RpcCallback& callback) -> kosio::async::Task<Result<void>>;

    [[REMEMBER_CO_AWAIT]]
    auto send_request(
        detail::ServiceType service_type,
        detail::MethodType method_type,
        std::string_view req_payload,
        RpcCallback&& callback) -> kosio::async::Task<Result<void>>;

    [[REMEMBER_CO_AWAIT]]
    auto shutdown() -> kosio::async::Task<void>;

private:
    [[REMEMBER_CO_AWAIT]]
    auto do_shutdown() -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto wait_shutdown() const -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto redirect_to(std::string_view resp_payload) -> kosio::async::Task<Result<void>>;

    [[REMEMBER_CO_AWAIT]]
    auto trigger_callback(uint64_t request_id, std::string_view resp_payload) -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_response() -> kosio::async::Task<void>;

private:
    kosio::net::TcpStream stream_;
    uint64_t              request_id_{0};
    RpcCallbackMap        callbacks_;
    kosio::sync::Mutex    mutex_;
    bool                  is_shutdown_{false};
};
} // namespace vosfs::rpc