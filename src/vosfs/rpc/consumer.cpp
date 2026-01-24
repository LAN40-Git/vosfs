#include "vosfs/rpc/consumer.hpp"

auto vosfs::rpc::RpcConsumer::create(std::string_view host, uint16_t port)
-> kosio::async::Task<Result<std::unique_ptr<RpcConsumer>>> {
    auto has_addr = kosio::net::SocketAddr::parse(host, port);
    if (!has_addr) {
        LOG_ERROR("{}", has_addr.error());
        co_return std::unexpected{make_error(Error::kInvalidServerAddress)};
    }
    auto has_stream = co_await kosio::net::TcpStream::connect(has_addr.value());
    if (!has_stream) {
        LOG_ERROR("{}", has_stream.error());
        co_return std::unexpected{make_error(Error::kConnectToServerFailed)};
    }
    co_return std::make_unique<RpcConsumer>(std::move(has_stream.value()));
}

auto vosfs::rpc::RpcConsumer::send_request(
    detail::ServiceType service_type,
    detail::MethodType method_type,
    std::string_view req_payload,
    const RpcCallback& callback) -> kosio::async::Task<Result<void>> {
    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

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
    detail::ServiceType service_type,
    detail::MethodType method_type,
    std::string_view req_payload,
    RpcCallback&& callback) -> kosio::async::Task<Result<void>> {
    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    // Make fixed request header
    detail::FixedRpcRequestHeader header;
    header.request_id = htobe64(request_id_);
    header.service_type = service_type;
    header.method_type = method_type;
    header.payload_size = htobe32(req_payload.size());

    callbacks_.emplace(request_id_++, std::move(callback));

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
