#include "vosfs/rpc/consumer.hpp"

auto vosfs::rpc::RpcConsumer::create(std::string_view host)
-> kosio::async::Task<Result<std::unique_ptr<RpcConsumer>>> {
    auto has_addr = kosio::net::SocketAddr::parse(host, detail::RPC_PORT);
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

    if (status_ == ShutDown) {
        co_return std::unexpected{make_error(Error::kConnectionShutdown)};
    }

    // Make fixed request header
    detail::FixedRpcRequestHeader req_header;
    req_header.request_id = htobe64(request_id_);
    req_header.payload_size = htobe32(req_payload.size());
    req_header.service_type = service_type;
    req_header.method_type = method_type;

    callbacks_.emplace(request_id_++, callback);

    auto ret = co_await stream_.write_vectored(
        std::span<const char>(reinterpret_cast<char*>(&req_header), sizeof(req_header)),
        std::span<const char>(req_payload.data(), req_payload.size())
    );

    if (!ret) {
        LOG_ERROR("{}", ret.error());
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

    if (status_ == ShutDown) {
        co_return std::unexpected{make_error(Error::kConsumerHasShutdown)};
    }

    // Make fixed request header
    detail::FixedRpcRequestHeader req_header;
    req_header.request_id = htobe64(request_id_);
    req_header.payload_size = htobe32(req_payload.size());
    req_header.service_type = service_type;
    req_header.method_type = method_type;

    callbacks_.emplace(request_id_++, std::move(callback));

    auto ret = co_await stream_.write_vectored(
        std::span<const char>(reinterpret_cast<char*>(&req_header), sizeof(req_header)),
        std::span<const char>(req_payload.data(), req_payload.size())
    );

    if (!ret) {
        LOG_ERROR("{}", ret.error());
        co_return std::unexpected{make_error(Error::kSendRpcRequestFailed)};
    }

    co_return Result<void>{};
}

auto vosfs::rpc::RpcConsumer::shutdown() -> kosio::async::Task<Result<void>> {
    {
        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);

        if (status_ == ShuttingDown || status_ == ShutDown) {
            co_return std::unexpected{make_error(Error::kConsumerHasShutdown)};
        }

        status_ = ShuttingDown;
    }

    if (auto ret = co_await send_shutdown_request(); !ret) {
        co_return ret;
    }

    co_await shutdown_latch_.wait();
    LOG_INFO("Consumer has shutdown.");
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
            {reinterpret_cast<char*>(&resp_header), sizeof(resp_header)});
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

        if (error_code == detail::RpcError::kShutdown) {
            co_await stream_.close();
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

        // Remove callback when rpc request not success
        if (error_code != detail::RpcError::kSuccess) {
            callbacks_.erase(request_id);
        }

        switch (error_code) {
            case detail::RpcError::kSuccess: {
                co_await trigger_callback(request_id, {buf.data(), payload_size});
                break;
            }
            case detail::RpcError::kNeedShutdown: {
                // Notify the server to stop reading and writing
                if (!co_await send_request(ServiceType::kConn, MethodType::kConnShutdown, "",
                    [](std::string_view) -> kosio::async::Task<void> {co_return;})) {
                    co_return;
                }
                break;
            }
            default: {
                LOG_ERROR("Rpc error : {}", detail::make_rpc_error(error_code));
                break;
            }
        }
    }

    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);
    status_ = ShutDown;
    shutdown_latch_.count_down();
}

auto vosfs::rpc::RpcConsumer::send_shutdown_request() -> kosio::async::Task<Result<void>> {
    co_return co_await send_request(ServiceType::kConn, MethodType::kConnShutdown, "",
                    [](std::string_view) -> kosio::async::Task<void> {co_return;});
}
