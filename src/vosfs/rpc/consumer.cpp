#include "vosfs/rpc/consumer.hpp"

auto vosfs::rpc::RpcConsumer::create(std::string_view server_ip, uint16_t server_port)
-> kosio::async::Task<Result<std::unique_ptr<RpcConsumer>>> {
    auto has_addr = kosio::net::SocketAddr::parse(server_ip, server_port);
    if (!has_addr) {
        co_return std::unexpected{make_error(Error::kCreateRpcConsumerFailed)};
    }
    auto server_addr_ = has_addr.value();
    co_return std::unique_ptr<RpcConsumer>(new RpcConsumer(server_addr_, kosio::net::TcpStream{kosio::net::detail::Socket{-1}}));
}

auto vosfs::rpc::RpcConsumer::connect() -> kosio::async::Task<void> {
    LOG_INFO("try to connect to {}", server_addr_);
    auto has_stream = co_await kosio::net::TcpStream::connect(server_addr_);
    if (!has_stream) {
        LOG_ERROR("{}", has_stream.error());
        co_return;
    }
    LOG_INFO("connect to {}", server_addr_);
    stream_ = std::move(has_stream.value());
    is_connected_ = true;
    kosio::spawn(handle_response_loop());
}

auto vosfs::rpc::RpcConsumer::shutdown() -> kosio::async::Task<void> {
    while (true) {
        co_await kosio::io::cancel(stream_.fd(), IORING_ASYNC_CANCEL_ALL);
        co_await kosio::time::sleep(50);
        co_await mutex_.lock();
        std::lock_guard lock{mutex_, std::adopt_lock};
        is_shutdown_ = true;
        if (!is_connected_) {
            break;
        }
    }
}

auto vosfs::rpc::RpcConsumer::trigger_callback(uint64_t request_id, std::string_view resp_payload) -> kosio::async::Task<void> {
    RpcCallback callback;
    {
        RpcCallbackMap::accessor acc;
        if (!callbacks_.find(acc, request_id)) {
            co_return;
        }
        callback = acc->second;
    }

    co_await callback(resp_payload);
    callbacks_.erase(request_id);
}

auto vosfs::rpc::RpcConsumer::handle_response_loop() -> kosio::async::Task<void> {
    std::vector<char> buf(detail::MAX_RPC_MESSAGE_SIZE);

    while (true) {
        // receive fixed response header
        detail::FixedRpcResponseHeader resp_header;
        auto ret = co_await stream_.read_exact(
            {reinterpret_cast<char*>(&resp_header), sizeof(resp_header)});
        if (!ret) {
            break;
        }

        auto request_id = be64toh(resp_header.request_id);
        auto payload_size = be32toh(resp_header.payload_size);
        auto status = resp_header.status;
        if (payload_size > detail::MAX_RPC_MESSAGE_SIZE) {
            LOG_ERROR("receive unusual rpc message, request_id: {}, payload_size: {}.", request_id, payload_size);
            callbacks_.erase(request_id);
            break;
        }

        if (payload_size > 0) {
            // receive response payload
            ret = co_await stream_.read_exact({buf.data(), payload_size});
            if (!ret) [[unlikely]] {
                LOG_ERROR("failed to receive response payload: {}", ret.error());
                break;
            }
        }

        if (status != RpcResult::kSuccess) {
            LOG_ERROR("rpc result: {}", make_result(status));
        }

        co_await trigger_callback(request_id, {buf.data(), payload_size});
    }
    co_await stream_.close();
    co_await mutex_.lock();
    std::lock_guard lock{mutex_, std::adopt_lock};
    is_connected_ = false;
}
