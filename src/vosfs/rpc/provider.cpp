#include "vosfs/rpc/provider.hpp"
#include <ranges>

auto vosfs::rpc::RpcProvider::create(uint16_t port, AuthMode auth_mode)
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
    auto provider = std::make_unique<RpcProvider>(port, auth_mode, std::move(has_listener.value()));
    provider->register_invoke(ServiceType::kConn, MethodType::kConnShutdown, [](
        std::string_view, std::span<char>) -> kosio::async::Task<InvokeResult> {
        co_return std::make_pair(RpcError::kShutdown, 0);
    });
    co_return std::move(provider);
}

void vosfs::rpc::RpcProvider::register_invoke(
    ServiceType service_type,
    MethodType method_type,
    const detail::Invoke& invoke) {
    invokes_[service_type][method_type] = invoke;
}

auto vosfs::rpc::RpcProvider::run() -> kosio::async::Task<Result<void>> {
    {
        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);

        if (!is_shutdown_) {
            co_return std::unexpected{make_error(Error::kProviderIsRunning)};
        }
    }

    is_listening_.store(true, std::memory_order_relaxed);

    while (true) {
        auto has_stream = co_await listener_.accept();
        if (!has_stream) {
            LOG_VERBOSE("Failed to accept rpc connection : {}", has_stream.error());
            break;
        }
        auto& [stream, peer_addr] = has_stream.value();

        auto session = session_manager_.assign_session(std::move(stream), peer_addr);
        LOG_VERBOSE("Accept connection from {}, session_id : {}", peer_addr, session->id);
        session->is_authorized = false;
        kosio::spawn(handle_request(session));
        kosio::spawn(send_response(session));
    }

    is_listening_.store(false, std::memory_order_relaxed);
    LOG_VERBOSE("The provider has stop listening.");
    co_return Result<void>{};
}

auto vosfs::rpc::RpcProvider::shutdown() -> kosio::async::Task<Result<void>> {
    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    // Stop listening
    while (is_listening_.load(std::memory_order_relaxed)) {
        auto has_cancle = co_await kosio::io::cancel(listener_.fd(), IORING_ASYNC_CANCEL_ALL);
        if (!has_cancle) {
            LOG_FATAL("Failed to stop listening : {}", has_cancle.error());
            co_return std::unexpected{make_error(Error::kStopListeningFailed)};
        }

        co_await kosio::time::sleep(50);
    }

    // Clear sessions
    for (const auto& session : session_manager_.sessions_ | std::views::values) {
        detail::InvokeTask invoke_task;
        invoke_task.error_code_ = RpcError::kNeedShutdown;
        co_await session->invoke_queue.push(std::move(invoke_task));
    }

    while (!session_manager_.sessions_.empty()) {
        co_await kosio::time::sleep(50);
    }

    LOG_VERBOSE("The sessions has been clean up.");
    is_shutdown_ = true;
    co_return Result<void>{};
}

auto vosfs::rpc::RpcProvider::handle_request(std::shared_ptr<detail::Session> session) -> kosio::async::Task<void> {
    auto& stream = session->stream;
    auto& invoke_queue = session->invoke_queue;
    std::vector<char> buf(detail::MAX_RPC_MESSAGE_SIZE);

    detail::FixedRpcRequestHeader req_header;

    while (true) {
        // Recv fixed request header
        auto ret = co_await stream.read_exact(
            {reinterpret_cast<char*>(&req_header), sizeof(req_header)});
        if (!ret) {
            LOG_ERROR("{}", ret.error());
            break;
        }

        auto request_id = be64toh(req_header.request_id);
        auto payload_size = be32toh(req_header.payload_size);
        auto service_type = req_header.service_type;
        auto method_type = req_header.method_type;

        if (service_type == ServiceType::kConn && method_type == MethodType::kConnShutdown) {
            break;
        }

        if (payload_size > detail::MAX_RPC_MESSAGE_SIZE) {
            LOG_ERROR("Receive unusual rpc message, request_id : {}, payload_size : {}.", request_id, payload_size);
            break;
        }

        // Recv req_payload
        ret = co_await stream.read_exact({buf.data(), payload_size});
        if (!ret) {
            LOG_ERROR("{}", ret.error());
            break;
        }

        detail::InvokeTask invoke_task;
        invoke_task.request_id_ = request_id;

        if (auth_mode_ == AuthMode::REQUIRED) {
            if (!session->is_authorized) {
                invoke_task.error_code_ = RpcError::kUnauthenticated;
                co_await invoke_queue.push(std::move(invoke_task));
                continue;
            }
        }

        // Get invoke
        auto service = invokes_.find(service_type);
        if (service == invokes_.end()) {
            invoke_task.error_code_ = RpcError::kFindServiceTypeFailed;
            co_await invoke_queue.push(std::move(invoke_task));
            continue;
        }

        auto invoke = service->second.find(method_type);
        if (invoke == service->second.end()) {
            invoke_task.error_code_ = RpcError::kFindMethodTypeFailed;
            co_await invoke_queue.push(std::move(invoke_task));
            continue;
        }

        invoke_task.req_payload_ = std::string{buf.data(), payload_size};
        invoke_task.invoke_ = invoke->second;

        co_await invoke_queue.push(std::move(invoke_task));
    }

    co_await invoke_queue.shutdown();
}

auto vosfs::rpc::RpcProvider::send_response(std::shared_ptr<detail::Session> session) -> kosio::async::Task<void> {
    auto& stream = session->stream;
    auto& invoke_queue = session->invoke_queue;
    std::vector<char> buf(detail::MAX_RPC_MESSAGE_SIZE);

    detail::FixedRpcResponseHeader resp_header;

    while (true) {
        // Get invoke task
        auto has_invoke_task = co_await invoke_queue.pop();
        if (!has_invoke_task) {
            resp_header.error_code = RpcError::kShutdown;
            co_await stream.write_all({reinterpret_cast<char*>(&resp_header), sizeof(resp_header)});
            break;
        }
        auto invoke_task = std::move(has_invoke_task.value());
        resp_header.request_id = htobe64(invoke_task.request_id_);
        resp_header.error_code = invoke_task.error_code_;

        if (invoke_task.error_code_ == RpcError::kSuccess) {
            auto ret = co_await invoke_task.invoke_(
                invoke_task.req_payload_,
                {buf.data(), buf.capacity()});
            resp_header.error_code = ret.first;
            resp_header.payload_size = htobe32(ret.second);
        }

        auto ret = co_await stream.write_vectored(
            std::span<const char>(reinterpret_cast<char*>(&resp_header), sizeof(resp_header)),
            std::span<const char>(buf.data(), be32toh(resp_header.payload_size))
        );

        if (!ret) {
            LOG_ERROR("Failed to send response to Session {} : {}", session->id, ret.error());
            break;
        }
    }

    // Delay 100ms before shutdown
    co_await kosio::time::sleep(100);
    session_manager_.remove_session(session->id);
}
