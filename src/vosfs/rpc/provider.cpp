#include "vosfs/rpc/provider.hpp"

#include <ranges>

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

        is_shutdown_ = false;
    }

    while (true) {
        auto has_stream = co_await listener_.accept();
        if (!has_stream) {
            LOG_VERBOSE("Failed to accept rpc connection : {}", has_stream.error());
            break;
        }
        auto& [stream, peer_addr] = has_stream.value();

        auto session = session_manager_.assign_unauth_session(std::move(stream), peer_addr);
        LOG_VERBOSE("Accept connection from {}, session_id : {}", peer_addr, session->id);
        kosio::spawn(handle_unauth_request(session));
        kosio::spawn(send_response(session));
    }

    LOG_VERBOSE("Stop listening");
    co_return Result<void>{};
}

auto vosfs::rpc::RpcProvider::shutdown() -> kosio::async::Task<Result<void>> {
    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    if (is_shutdown_) {
        co_return std::unexpected{make_error(Error::kProviderHasShutdown)};
    }

    is_shutdown_ = true;

    // Stop listening
    auto has_cancle = co_await kosio::io::cancel(listener_.fd(), IORING_ASYNC_CANCEL_ALL);
    if (!has_cancle) {
        LOG_FATAL("Failed to stop listening : {}", has_cancle.error());
        co_return std::unexpected{make_error(Error::kStopListeningFailed)};
    }

    co_await this->remind_all_sessions_shutdown();

    while (!session_manager_.unauth_sessions_.empty()
        || !session_manager_.auth_sessions_.empty()) {
        co_await kosio::time::sleep(50);
    }

    LOG_VERBOSE("The sessions has been clean up");

    co_return Result<void>{};
}

auto vosfs::rpc::RpcProvider::handle_unauth_request(std::shared_ptr<detail::Session> session) -> kosio::async::Task<void> {
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

        detail::InvokeTask invoke_task;
        invoke_task.request_id_ = request_id;

        // Recv req_payload
        ret = co_await stream.read_exact({buf.data(), payload_size});
        if (!ret) {
            LOG_ERROR("{}", ret.error());
            break;
        }

        // if (!is_unauth_request(service_type, method_type)) {
        //     invoke_task.error_code_ = detail::RpcError::kUnauthenticated;
        //     co_await invoke_queue.push(std::move(invoke_task));
        //     continue;
        // }

        // Get invoke
        auto service = invokes_.find(service_type);
        if (service == invokes_.end()) {
            invoke_task.error_code_ = detail::RpcError::kFindServiceTypeFailed;
            co_await invoke_queue.push(std::move(invoke_task));
            continue;
        }

        auto invoke = service->second.find(method_type);
        if (invoke == service->second.end()) {
            invoke_task.error_code_ = detail::RpcError::kFindMethodTypeFailed;
            co_await invoke_queue.push(std::move(invoke_task));
            continue;
        }

        invoke_task.req_payload_ = std::string{buf.data(), payload_size};
        invoke_task.invoke_ = invoke->second;

        co_await invoke_queue.push(std::move(invoke_task));
    }

    co_await invoke_queue.shutdown();
}

auto vosfs::rpc::RpcProvider::handle_auth_request(std::shared_ptr<detail::Session> session) -> kosio::async::Task<void> {
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

        // Get invoke
        auto service = invokes_.find(service_type);
        if (service == invokes_.end()) {
            invoke_task.error_code_ = detail::RpcError::kFindServiceTypeFailed;
            co_await invoke_queue.push(std::move(invoke_task));
            continue;
        }

        auto invoke = service->second.find(method_type);
        if (invoke == service->second.end()) {
            invoke_task.error_code_ = detail::RpcError::kFindMethodTypeFailed;
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
            resp_header.error_code = detail::RpcError::kShutdown;
            co_await stream.write_all({reinterpret_cast<char*>(&resp_header), sizeof(resp_header)});
            break;
        }
        auto invoke_task = std::move(has_invoke_task.value());

        resp_header.request_id = htobe64(invoke_task.request_id_);

        switch (invoke_task.error_code_) {
            case detail::RpcError::kSuccess: {
                // Do invoke and get resp_payload size
                auto has_resp_payload_size = co_await invoke_task.invoke_(
                    invoke_task.req_payload_,
                    {buf.data(), buf.capacity()});

                if (!has_resp_payload_size) {
                    if (has_resp_payload_size.error().value() == Error::kNeedRedirect) {
                        resp_header.error_code = detail::RpcError::kRedirect;
                    } else {
                        resp_header.error_code = detail::RpcError::kGetRespPayloadFailed;
                    }
                    break;
                }
                resp_header.payload_size = htobe32(has_resp_payload_size.value());
                break;
            }
            default: {
                resp_header.error_code = invoke_task.error_code_;
            }
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

    session_manager_.remove_session(session->is_auth, session->id);
}

auto vosfs::rpc::RpcProvider::remind_all_sessions_shutdown() -> kosio::async::Task<void> {
    for (const auto& session : session_manager_.unauth_sessions_ | std::views::values) {
        detail::InvokeTask invoke_task;
        invoke_task.error_code_ = detail::RpcError::kNeedShutdown;
        co_await session->invoke_queue.push(std::move(invoke_task));
    }

    for (const auto& session : session_manager_.auth_sessions_ | std::views::values) {
        detail::InvokeTask invoke_task;
        invoke_task.error_code_ = detail::RpcError::kNeedShutdown;
        co_await session->invoke_queue.push(std::move(invoke_task));
    }
}

auto vosfs::rpc::RpcProvider::is_unauth_request(ServiceType service_type, MethodType method_type) -> bool {
    if (service_type != ServiceType::kAuth && service_type != ServiceType::kConn) {
        return false;
    }

    if (method_type == MethodType::kAuthSignout) {
        return false;
    }

    return true;
}
