#include "vosfs/raft/internal/peer.hpp"

vosfs::raft::detail::Peer::Peer(
    uint64_t member_id, std::string_view name, std::string_view host,
    std::unique_ptr<rpc::RpcConsumer> consumer)
    : member_id_(member_id)
    , name_(name)
    , host_(host)
    , consumer_(std::move(consumer)) {}

auto vosfs::raft::detail::Peer::create(uint64_t member_id, std::string_view name,
    std::string_view host) -> kosio::async::Task<Result<Peer>> {
    // try to connect to the raft node at host:RAFT:PROVIDER_PORT
    auto ret = co_await rpc::RpcConsumer::create(host, RAFT_PROVIDER_PORT);
    if (!ret) {
        co_return std::unexpected{make_error(Error::kCreatePeerFailed)};
    }
    co_return Peer(member_id, name, host, std::move(ret.value()));
}

auto vosfs::raft::detail::Peer::shutdown() const -> kosio::async::Task<Result<void>> {
    co_return co_await consumer_->shutdown();
}

auto vosfs::raft::detail::Peer::send_request(
    rpc::ServiceType service_type,
    rpc::MethodType method_type,
    std::string_view req_payload,
    const rpc::RpcCallback& callback) const -> kosio::async::Task<Result<void>> {
    co_return co_await consumer_->send_request(service_type, method_type, req_payload, callback);
}

auto vosfs::raft::detail::Peer::send_request(
    rpc::ServiceType service_type,
    rpc::MethodType method_type,
    std::string&& req_payload,
    rpc::RpcCallback&& callback) const -> kosio::async::Task<Result<void>> {
    co_return co_await consumer_->send_request(service_type, method_type, std::move(req_payload), std::move(callback));
}
