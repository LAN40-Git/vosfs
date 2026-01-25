#include "vosfs/rpc/provider.hpp"

auto vosfs::rpc::RpcProvider::create(uint16_t port)
-> kosio::async::Task<Result<std::unique_ptr<RpcProvider>>> {
    auto has_addr = kosio::net::SocketAddr::parse("0.0.0.0", port);
    if (!has_addr) {
        LOG_ERROR("Failed to create rpc provider : {}", has_addr.error());
        co_return std::unexpected{make_error(Error::kInvalidAddress)};
    }
    auto has_listener = kosio::net::TcpListener::bind(has_addr.value());
    if (!has_listener) {
        LOG_ERROR("Failed to create rpc provider : {}", has_listener.error());
        co_return std::unexpected{make_error(Error::kBindFailed)};
    }
    co_return std::make_unique<RpcProvider>(port, std::move(has_listener.value()));
}

void vosfs::rpc::RpcProvider::register_invoke(
    detail::ServiceType service_type,
    detail::MethodType method_type,
    const detail::Invoke& invoke) {
    invokes_[service_type][method_type] = invoke;
}

auto vosfs::rpc::RpcProvider::handle_connection() -> kosio::async::Task<void> {
    while (true) {
        auto has_stream = co_await listener_.accept();
        if (!has_stream) [[unlikely]] {
            LOG_VERBOSE("Failed to accept rpc connection : {}", has_stream.error());
            break;
        }
        auto& [stream, peer_addr] = has_stream.value();
    }
}
