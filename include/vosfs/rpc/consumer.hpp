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
    using RpcCallbackMap = tbb::concurrent_hash_map<uint64_t, RpcCallback>;

private:
    explicit RpcConsumer(std::string_view server_ip, uint16_t server_port)
        : server_ip_(server_ip)
        , server_port_(server_port)
        , stream_(kosio::net::TcpStream{kosio::net::detail::Socket{-1}})
        , request_buf_(detail::MAX_RPC_MESSAGE_SIZE) { callbacks_.rehash(4096); }

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
    template <typename Request>
    [[REMEMBER_CO_AWAIT]]
    auto send_request(
        ServiceType service_type,
        MethodType method_type,
        const Request& request,
        const RpcCallback& callback) -> kosio::async::Task<void> {
        if (is_shutdown()) {
            co_await run();
        }

        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);

        if (is_shutdown_.load(std::memory_order_relaxed)) {
            LOG_ERROR("failed to send rpc request: consumer has shutdown");
            co_return;
        }

        auto payload_size = request.ByteSizeLong();

        if (!request.SerializeToArray(request_buf_.data(), static_cast<int>(payload_size))) {
            LOG_ERROR("failed to serialize request");
            co_return;
        }

        detail::FixedRpcRequestHeader req_header;
        req_header.request_id = htobe64(request_id_);
        req_header.payload_size = htobe32(payload_size);
        req_header.service_type = service_type;
        req_header.method_type = method_type;

        auto ret = co_await stream_.write_vectored(
            std::span<const char>(reinterpret_cast<char*>(&req_header), sizeof(req_header)),
            std::span<const char>(request_buf_.data(), payload_size)
        );

        if (!ret) {
            LOG_ERROR("failed to send rpc request: {}", ret.error());
            co_return;
        }

        callbacks_.emplace(request_id_++, callback);
    }

    template <typename Request>
    [[REMEMBER_CO_AWAIT]]
    auto send_request(
        ServiceType service_type,
        MethodType method_type,
        const Request& request,
        RpcCallback&& callback) -> kosio::async::Task<void> {
        if (is_shutdown()) {
            co_await run();
        }

        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);

        if (is_shutdown_.load(std::memory_order_relaxed)) {
            LOG_ERROR("failed to send rpc request: consumer has shutdown");
            co_return;
        }

        auto payload_size = request.ByteSizeLong();

        if (!request.SerializeToArray(request_buf_.data(), static_cast<int>(payload_size))) {
            LOG_ERROR("failed to serialize request");
            co_return;
        }

        detail::FixedRpcRequestHeader req_header;
        req_header.request_id = htobe64(request_id_);
        req_header.payload_size = htobe32(payload_size);
        req_header.service_type = service_type;
        req_header.method_type = method_type;

        auto ret = co_await stream_.write_vectored(
            std::span<const char>(reinterpret_cast<char*>(&req_header), sizeof(req_header)),
            std::span<const char>(request_buf_.data(), payload_size)
        );

        if (!ret) {
            LOG_ERROR("failed to send rpc request: {}", ret.error());
            co_return;
        }

        callbacks_.emplace(request_id_++, std::move(callback));
    }

private:
    [[REMEMBER_CO_AWAIT]]
    void remove_callback(uint64_t request_id);

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
    std::vector<char>     request_buf_;
};

using RpcClient = std::unique_ptr<RpcConsumer>;
} // namespace vosfs::rpc