#include "vosfs/raft/internal/peer.hpp"

auto vosfs::raft::detail::Peer::create(const RaftNodeInfo& raft_node_info) -> kosio::async::Task<Result<Peer>> {
    // try to connect to the raft node at ip:RAFT_RPC_PORT
    auto ret = co_await rpc::RpcConsumer::create(raft_node_info.ip(), RAFT_RPC_PORT);
    if (!ret) {
        co_return std::unexpected{make_error(Error::kCreatePeerFailed)};
    }
    co_return Peer(raft_node_info, std::move(ret.value()));
}

auto vosfs::raft::detail::Peer::shutdown() const -> kosio::async::Task<void> {
    co_await consumer_->shutdown();
}

auto vosfs::raft::detail::Peer::send_request(
    rpc::ServiceType service_type,
    rpc::MethodType method_type,
    std::string_view req_payload,
    const rpc::RpcCallback& callback) const -> kosio::async::Task<void> {
    if (consumer_->is_shutdown()) {
        co_await consumer_->run();
    }
    co_await consumer_->send_request(service_type, method_type, req_payload, callback);
}

auto vosfs::raft::detail::Peer::send_request(
    rpc::ServiceType service_type,
    rpc::MethodType method_type,
    std::string&& req_payload,
    rpc::RpcCallback&& callback) const -> kosio::async::Task<void> {
    if (consumer_->is_shutdown()) {
        co_await consumer_->run();
    }
    co_await consumer_->send_request(service_type, method_type, std::move(req_payload), std::move(callback));
}
