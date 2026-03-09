#include "vosfs/rpc/consumer.hpp"

auto vosfs::rpc::RpcConsumer::create(std::string_view host, uint16_t port)
-> kosio::async::Task<Result<std::unique_ptr<RpcConsumer>>> {
    auto has_addr = kosio::net::SocketAddr::parse(host, port);
    if (!has_addr) {
        LOG_ERROR("Failed to create rpc consumer : {}", has_addr.error());
        co_return std::unexpected{make_error(Error::kInvalidAddress)};
    }
    auto has_stream = co_await kosio::net::TcpStream::connect(has_addr.value());
    if (!has_stream) {
        LOG_ERROR("{}", has_stream.error());
        co_return std::unexpected{make_error(Error::kConnectToServerFailed)};
    }

    auto consumer = std::make_unique<RpcConsumer>(std::move(has_stream.value()));
    consumer->is_handle_response_.store(true, std::memory_order_relaxed);
    kosio::spawn(consumer->handle_response());
    co_return std::move(consumer);
}

auto vosfs::rpc::RpcConsumer::send_request(
    ServiceType service_type,
    MethodType method_type,
    std::string_view req_payload,
    const RpcCallback& callback) -> kosio::async::Task<Result<void>> {
    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    if (is_shutdown_) {
        co_return std::unexpected{make_error(Error::kConnectionShutdown)};
    }

    // Make fixed request header
    detail::FixedRpcRequestHeader header;
    header.request_id = htobe64(request_id_);
    header.service_type = service_type;
    header.method_type = method_type;
    header.payload_size = htobe32(req_payload.size());

    callbacks_.emplace(request_id_++, callback);

    auto ret = co_await stream_.write_vectored(
        std::span<const char>(reinterpret_cast<char*>(&header), sizeof(header)),
        std::span<const char>(req_payload.data(), req_payload.size())
    );

    if (!ret) {
        LOG_ERROR("{}", ret.error());
        co_await stream_.close();
        co_return std::unexpected{make_error(Error::kSendRpcRequestFailed)};
    }

    co_return Result<void>{};
}

auto vosfs::rpc::RpcConsumer::send_request(
    ServiceType service_type,
    MethodType method_type,
    std::string_view req_payload,
    RpcCallback&& callback) -> kosio::async::Task<Result<void>> {
    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    if (is_shutdown_) {
        co_return std::unexpected{make_error(Error::kConnectionShutdown)};
    }

    // Make fixed request header
    detail::FixedRpcRequestHeader req_header;
    req_header.request_id = htobe64(request_id_);
    req_header.service_type = service_type;
    req_header.method_type = method_type;
    req_header.payload_size = htobe32(req_payload.size());

    callbacks_.emplace(request_id_++, std::move(callback));

    auto ret = co_await stream_.write_vectored(
        std::span<const char>(reinterpret_cast<char*>(&req_header), sizeof(req_header)),
        std::span<const char>(req_payload.data(), req_payload.size())
    );

    if (!ret) {
        LOG_ERROR("{}", ret.error());
        co_await stream_.close();
        co_return std::unexpected{make_error(Error::kSendRpcRequestFailed)};
    }

    co_return Result<void>{};
}

auto vosfs::rpc::RpcConsumer::shutdown() -> kosio::async::Task<Result<void>> {
    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    if (is_shutdown_) {
        co_return std::unexpected{make_error(Error::kConsumerHasShutdown)};
    }

    // Stop handling response
    while (is_handle_response_.load(std::memory_order_relaxed)) {
        auto has_cancle = co_await kosio::io::cancel(stream_.fd(), IORING_ASYNC_CANCEL_ALL);
        if (!has_cancle) {
            LOG_FATAL("{}", has_cancle.error());
            break;
        }

        co_await kosio::time::sleep(50);
    }

    is_shutdown_ = true;
    co_return Result<void>{};
}

auto vosfs::rpc::RpcConsumer::redirect_to(std::string_view resp_payload) -> kosio::async::Task<Result<void>> {
    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    RedirectInfo redirect_info;
    if (!redirect_info.ParseFromArray(resp_payload.data(), resp_payload.size())) {
        co_return std::unexpected{make_error(Error::kParseRpcMessageFailed)};
    }

    auto host = redirect_info.host();
    auto port = static_cast<uint16_t>(redirect_info.port());

    co_await stream_.close();

    auto has_addr = kosio::net::SocketAddr::parse(host, port);
    if (!has_addr) {
        LOG_ERROR("Failed to redirect to {}:{} : {}", host, port, has_addr.error());
        co_return std::unexpected{make_error(Error::kInvalidAddress)};
    }
    auto has_stream = co_await kosio::net::TcpStream::connect(has_addr.value());
    if (!has_stream) {
        LOG_ERROR("{}", has_stream.error());
        co_return std::unexpected{make_error(Error::kConnectToServerFailed)};
    }

    stream_ = std::move(has_stream.value());
    co_return Result<void>{};
}

auto vosfs::rpc::RpcConsumer::trigger_callback(uint64_t request_id, std::string_view resp_payload) -> kosio::async::Task<void> {
    tbb::concurrent_hash_map<uint64_t, RpcCallback>::accessor acc;
    if (callbacks_.find(acc, request_id)) {
        auto callback = std::move(acc->second);
        acc.release();
        co_await callback(resp_payload);
        callbacks_.erase(request_id);
    }
}

auto vosfs::rpc::RpcConsumer::handle_response() -> kosio::async::Task<void> {
    std::vector<char> buf(detail::MAX_RPC_MESSAGE_SIZE);

    while (true) {
        // Recv fixed response header
        detail::FixedRpcResponseHeader resp_header;
        auto ret = co_await stream_.read_exact(
            {reinterpret_cast<char*>(&resp_header), sizeof(detail::FixedRpcResponseHeader)});
        if (!ret) {
            LOG_ERROR("{}", ret.error());
            break;
        }

        auto request_id = be64toh(resp_header.request_id);
        auto payload_size = be32toh(resp_header.payload_size);
        auto error_code = resp_header.error_code;
        if (payload_size > detail::MAX_RPC_MESSAGE_SIZE) {
            LOG_ERROR("Receive unusual rpc message, request_id : {}, payload_size : {}.", request_id, payload_size);
            callbacks_.erase(request_id);
            break;
        }

        if (payload_size > 0) {
            // Recv response payload
            ret = co_await stream_.read_exact({buf.data(), payload_size});
            if (!ret) [[unlikely]] {
                LOG_ERROR("Failed to receive response payload : {}", ret.error());
                break;
            }
        }

        switch (error_code) {
            case detail::RpcError::kSuccess: {
                co_await trigger_callback(request_id, {buf.data(), payload_size});
                break;
            }
            case detail::RpcError::kRedirect: {
                auto has_redirect = co_await redirect_to({buf.data(), payload_size});
                if (!has_redirect) {
                    LOG_ERROR("Failed to redirect : {}", has_redirect.error());
                    break;
                }
                break;
            }
            default: {
                LOG_ERROR("Rpc error : {}", detail::make_rpc_error(error_code));
                break;
            }
        }
    }

    is_handle_response_.store(false, std::memory_order_relaxed);
}
