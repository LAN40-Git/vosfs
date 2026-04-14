#include "vosfs/rpc/consumer.hpp"

auto vosfs::rpc::RpcConsumer::create(std::string_view server_host, uint16_t server_port)
-> kosio::async::Task<Result<std::unique_ptr<RpcConsumer>>> {
    auto has_addr = kosio::net::SocketAddr::parse(server_host, server_port);
    if (!has_addr) {
        LOG_ERROR("Failed to create rpc consumer : {}", has_addr.error());
        co_return std::unexpected{make_error(Error::kInvalidAddress)};
    }
    auto has_stream = co_await kosio::net::TcpStream::connect(has_addr.value());
    if (!has_stream) {
        LOG_ERROR("{}", has_stream.error());
        co_return std::unexpected{make_error(Error::kConnectToServerFailed)};
    }

    auto consumer = std::unique_ptr<RpcConsumer>(new RpcConsumer(server_host, server_port, std::move(has_stream.value())));
    kosio::spawn(consumer->handle_response());
    co_return std::move(consumer);
}

auto vosfs::rpc::RpcConsumer::send_request(
    ServiceType service_type,
    MethodType method_type,
    std::string_view req_payload,
    const RpcCallback& callback) -> kosio::async::Task<Result<void>> {
    co_return co_await send_request_impl(service_type, method_type, std::string{req_payload}, RpcCallback{callback});
}

auto vosfs::rpc::RpcConsumer::send_request(
    ServiceType service_type,
    MethodType method_type,
    std::string&& req_payload,
    RpcCallback&& callback) -> kosio::async::Task<Result<void>> {
    co_return co_await send_request_impl(service_type, method_type, std::move(req_payload), std::move(callback));
}

auto vosfs::rpc::RpcConsumer::run() -> kosio::async::Task<void> {
    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);
    if (!is_shutdown_.load(std::memory_order_relaxed)) {
        co_return;
    }

    auto has_addr = kosio::net::SocketAddr::parse(server_host_, server_port_);
    assert(has_addr);
    auto has_stream = co_await kosio::net::TcpStream::connect(has_addr.value());
    if (!has_stream) {
        LOG_ERROR("{}", has_stream.error());
        co_return;
    }
    stream_ = std::move(has_stream.value());
    is_shutdown_.store(false, std::memory_order_release);
    kosio::spawn(handle_response());
}

auto vosfs::rpc::RpcConsumer::shutdown() -> kosio::async::Task<void> {
    is_shutdown_.store(true, std::memory_order_release);

    while (stream_.is_valid()) {
        if (auto ret = co_await kosio::io::cancel(stream_.fd(), IORING_ASYNC_CANCEL_ALL); !ret) {
            LOG_ERROR("{}", ret.error());
        }

        co_await kosio::time::sleep(50);
    }
}

auto vosfs::rpc::RpcConsumer::is_shutdown() const -> bool {
    return is_shutdown_.load(std::memory_order_acquire);
}

auto vosfs::rpc::RpcConsumer::send_request_impl(
    ServiceType service_type,
    MethodType method_type,
    std::string&& req_payload,
    RpcCallback&& callback) -> kosio::async::Task<Result<void>> {
    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    if (is_shutdown_.load(std::memory_order_relaxed)) {
        co_return std::unexpected{make_error(Error::kConsumerShutdown)};
    }

    // Make fixed request header
    detail::FixedRpcRequestHeader req_header;
    req_header.request_id = htobe64(request_id_);
    req_header.payload_size = htobe32(req_payload.size());
    req_header.service_type = service_type;
    req_header.method_type = method_type;

    auto ret = co_await stream_.write_vectored(
        std::span<const char>(reinterpret_cast<char*>(&req_header), sizeof(req_header)),
        std::span<const char>(req_payload.data(), be32toh(req_header.payload_size))
    );

    requests_.emplace(request_id_++, RpcRequest{service_type, method_type, std::move(req_payload), std::move(callback)});

    if (!ret) {
        LOG_ERROR("{}", ret.error());
        co_return std::unexpected{make_error(Error::kSendRpcRequestFailed)};
    }

    co_return Result<void>{};
}

auto vosfs::rpc::RpcConsumer::trigger_callback(uint64_t request_id, std::string_view resp_payload) -> kosio::async::Task<void> {
    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    auto it = requests_.find(request_id);
    if (it != requests_.end()) {
        auto request = it->second;
        co_await request.callback(resp_payload);
        requests_.erase(request_id);
    }
}

auto vosfs::rpc::RpcConsumer::remove_request(uint64_t request_id) -> kosio::async::Task<void> {
    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);
    requests_.erase(request_id);
}

auto vosfs::rpc::RpcConsumer::handle_response() -> kosio::async::Task<void> {
    std::vector<char> buf(detail::MAX_RPC_MESSAGE_SIZE);

    while (!is_shutdown_.load(std::memory_order_acquire)) {
        // receive fixed response header
        detail::FixedRpcResponseHeader resp_header;
        auto ret = co_await stream_.read_exact(
            {reinterpret_cast<char*>(&resp_header), sizeof(resp_header)});
        if (!ret) {
            LOG_ERROR("{}", ret.error());
            break;
        }

        auto request_id = be64toh(resp_header.request_id);
        auto payload_size = be32toh(resp_header.payload_size);
        auto status = resp_header.status;
        if (payload_size > detail::MAX_RPC_MESSAGE_SIZE) {
            LOG_ERROR("Receive unusual rpc message, request_id : {}, payload_size : {}.", request_id, payload_size);
            co_await remove_request(request_id);
            break;
        }

        if (payload_size > 0) {
            // receive response payload
            ret = co_await stream_.read_exact({buf.data(), payload_size});
            if (!ret) [[unlikely]] {
                LOG_ERROR("Failed to receive response payload : {}", ret.error());
                break;
            }
        }

        // remove callback when rpc request not success
        if (status != RpcResult::kSuccess) {
            co_await remove_request(request_id);
        }

        switch (status) {
            case RpcResult::kSuccess: {
                co_await trigger_callback(request_id, {buf.data(), payload_size});
                break;
            }
            default: {
                LOG_ERROR("Rpc result : {}", make_result(status));
                break;
            }
        }
    }

    if (auto ret = co_await stream_.close(); !ret) {
        LOG_ERROR("{}", ret.error());
    }
    is_shutdown_.store(true, std::memory_order_release);
}
