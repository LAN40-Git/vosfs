#pragma once
#include "vosfs/rpc/request.hpp"

namespace vosfs::rpc {
class RpcConsumer {
    using RpcCallbackMap = tbb::concurrent_hash_map<uint64_t, RpcCallback>;

public:
    explicit RpcConsumer(kosio::net::TcpStream&& stream)
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

private:
    kosio::net::TcpStream stream_;
    uint64_t              request_id_{0};
    RpcCallbackMap        callbacks_;
    kosio::sync::Mutex    mutex_;
};
} // namespace vosfs::rpc