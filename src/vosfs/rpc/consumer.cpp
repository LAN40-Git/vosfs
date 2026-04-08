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

auto vosfs::rpc::RpcConsumer::run() -> kosio::async::Task<Result<void>> {
    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);
    if (status_ != ShutDown) {
        co_return std::unexpected{make_error(Error::kConsumerRunning)};
    }

    auto has_addr = kosio::net::SocketAddr::parse(server_host_, server_port_);
    if (!has_addr) {
        LOG_ERROR("Failed to create rpc consumer : {}", has_addr.error());
        co_return std::unexpected{make_error(Error::kInvalidAddress)};
    }
    auto has_stream = co_await kosio::net::TcpStream::connect(has_addr.value());
    if (!has_stream) {
        LOG_ERROR("{}", has_stream.error());
        co_return std::unexpected{make_error(Error::kConnectToServerFailed)};
    }
    stream_ = std::move(has_stream.value());
    kosio::spawn(handle_response());
    status_ = Running;
    co_return Result<void>{};
}

auto vosfs::rpc::RpcConsumer::shutdown() -> kosio::async::Task<Result<void>> {
    {
        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);

        if (status_ != Running) {
            co_return std::unexpected{make_error(Error::kConsumerNotRunning)};
        }

        status_ = ShuttingDown;
    }

    if (auto ret = co_await send_shutdown_request(); !ret) {
        co_return ret;
    }

    while (true) {
        co_await kosio::time::sleep(50);
        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);
        if (status_ == ShutDown) {
            break;
        }
    }
    LOG_INFO("Consumer has shutdown.");
    co_return Result<void>{};
}

auto vosfs::rpc::RpcConsumer::is_running() -> kosio::async::Task<bool> {
    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    co_return status_ == Running;
}

auto vosfs::rpc::RpcConsumer::send_request_impl(
    ServiceType service_type,
    MethodType method_type,
    std::string&& req_payload,
    RpcCallback&& callback) -> kosio::async::Task<Result<void>> {
    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    if (status_ == ShutDown) {
        co_return std::unexpected{make_error(Error::kConsumerNotRunning)};
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
    bool reconn{false};

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
        auto status = resp_header.status;
        if (payload_size > detail::MAX_RPC_MESSAGE_SIZE) {
            LOG_ERROR("Receive unusual rpc message, request_id : {}, payload_size : {}.", request_id, payload_size);
            co_await remove_request(request_id);
            break;
        }

        if (status == RpcResult::kShutdown) {
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
        if (status != RpcResult::kSuccess) {
            co_await remove_request(request_id);
        }

        switch (status) {
            case RpcResult::kSuccess: {
                co_await trigger_callback(request_id, {buf.data(), payload_size});
                break;
            }
            case RpcResult::kRedirect: {
                reconn = true;
                RedirectInfo info;
                if (!info.ParseFromArray(buf.data(), static_cast<int>(payload_size))) {
                    LOG_ERROR("Failed to redirect to new server.");
                }
                server_host_ = info.host();
                server_port_ = info.port();
                // notify the server to stop reading and writing
                if (!co_await send_shutdown_request()) {
                    co_return;
                }
                break;
            }
            case RpcResult::kNeedShutdown: {
                // notify the server to stop reading and writing
                if (!co_await send_shutdown_request()) {
                    co_return;
                }
                break;
            }
            default: {
                LOG_ERROR("Rpc result : {}", make_result(status));
                break;
            }
        }
    }

    co_await stream_.close();
    {
        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);
        status_ = ShutDown;
    }


    if (reconn) {
        std::size_t n{0};
        while (n++ < detail::RECONN_RETRY_TIMES) {
            if (auto ret = co_await run(); ret) {
                break;
            }
        }
    }
}

auto vosfs::rpc::RpcConsumer::send_shutdown_request() -> kosio::async::Task<Result<void>> {
    co_return co_await send_request(ServiceType::kConn, MethodType::kConnShutdown, "",
                    [](std::string_view) -> kosio::async::Task<void> {co_return;});
}
