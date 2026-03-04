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
        LOG_VERBOSE("Accept connection from {}", peer_addr);
        // assign a session
        session_manager_.assign_session(std::move(stream), peer_addr);
    }
}

auto vosfs::rpc::RpcProvider::handle_request(std::shared_ptr<detail::Session> session) -> kosio::async::Task<void> {
    auto& stream = session->stream;
    auto& invoke_queue = session->invoke_queue;
    std::vector<char> buf(detail::MAX_RPC_MESSAGE_SIZE);

    detail::FixedRpcRequestHeader req_header;

    while (true) {
        // Recv fixed request header
        auto ret = co_await stream.read_exact(
            {reinterpret_cast<char*>(&req_header), sizeof(detail::FixedRpcRequestHeader)});
        if (!ret) {
            LOG_ERROR("{}", ret.error());
            break;
        }


    }

    while (true) {
        detail::FixedRpcResponseHeader resp_header;

        // Recv fixed request header
        detail::FixedRpcRequestHeader req_header;
        auto ret = co_await stream.read_exact(
            {reinterpret_cast<char*>(&req_header), sizeof(detail::FixedRpcRequestHeader)});
        if (!ret) {
            LOG_ERROR("{}", ret.error());
            resp_header.error_code = detail::RpcError::kShutdown;
            ret = co_await stream.write_all({reinterpret_cast<char*>(&resp_header), sizeof(detail::FixedRpcResponseHeader)});
            break;
        }

        auto request_id = be64toh(req_header.request_id);
        auto service_type = req_header.service_type;
        auto method_type = req_header.method_type;

        resp_header.request_id = request_id;

        // Get invoke
        auto service = invokes_.find(service_type);
        if (service == invokes_.end()) {
            LOG_ERROR("Failed to find service type : {}", RpcType::to_string(service_type));
            resp_header.error_code = detail::RpcError::kFindServiceTypeFailed;
        }

        auto invoke = service->second.find(method_type);
        if (invoke == service->second.end()) {
            LOG_ERROR("Failed to find method type : {}", RpcType::to_string(method_type));
            resp_header.error_code = detail::RpcError::kFindMethodTypeFailed;
        }

        auto payload_size = be32toh(req_header.payload_size);

        if (payload_size > detail::MAX_RPC_MESSAGE_SIZE) {
            LOG_ERROR("Receive unusual rpc message, request_id : {}, payload_size : {}.", request_id, payload_size);
            break;
        }

        // Recv request payload
        ret = co_await stream.read_exact({buf.data(), payload_size});
        if (!ret) {
            LOG_ERROR("{}", ret.error());
            resp_header.error_code = detail::RpcError::kShutdown;
            ret = co_await stream.write_all({reinterpret_cast<char*>(&resp_header), sizeof(detail::FixedRpcResponseHeader)});
            break;
        }

        co_await invoke->second();
    }
}

auto vosfs::rpc::RpcProvider::send_response(std::shared_ptr<detail::Session> session) -> kosio::async::Task<void> {
    auto& stream = session->stream;
    auto& invoke_queue = session->invoke_queue;
    std::vector<char> buf(detail::MAX_RPC_MESSAGE_SIZE);
}
