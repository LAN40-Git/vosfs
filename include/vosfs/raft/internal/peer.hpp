#pragma once
#include <cstdint>
#include "vosfs/api/serverpb/raft.pb.h"
#include "vosfs/rpc/consumer.hpp"
#include "config.hpp"

namespace vosfs::raft::detail {
class Peer {
private:
    explicit Peer(const RaftNodeInfo& raft_node_info, std::unique_ptr<rpc::RpcConsumer> consumer)
        : raft_node_info_(raft_node_info), consumer_(std::move(consumer)) {}

public:
    Peer(const Peer&) = delete;
    auto operator=(const Peer&) -> Peer& = delete;
    Peer(Peer&& other) noexcept = default;
    auto operator=(Peer&&) noexcept -> Peer& = default;

public:
    [[nodiscard]]
    auto member_id() const noexcept -> uint64_t { return raft_node_info_.id(); }
    [[nodiscard]]
    auto name() const noexcept -> std::string_view { return raft_node_info_.name(); }
    [[nodiscard]]
    auto ip() const noexcept -> std::string_view { return raft_node_info_.ip(); }

public:
    [[REMEMBER_CO_AWAIT]]
    static auto create(const RaftNodeInfo& raft_node_info) -> kosio::async::Task<Result<Peer>> {
        // try to connect to the raft node at ip:RAFT_RPC_PORT
        auto ret = co_await rpc::RpcConsumer::create(raft_node_info.ip(), RAFT_RPC_PORT);
        if (!ret) {
            co_return std::unexpected{make_error(Error::kCreatePeerFailed)};
        }
        co_return Peer(raft_node_info, std::move(ret.value()));
    }

public:
    [[REMEMBER_CO_AWAIT]]
    auto shutdown() const -> kosio::async::Task<void> {
        co_await consumer_->shutdown();
    }

public:
    template <typename Request>
    [[REMEMBER_CO_AWAIT]]
    auto send_request(
        rpc::ServiceType service_type,
        rpc::MethodType method_type,
        const Request& request,
        const rpc::RpcCallback& callback) -> kosio::async::Task<void> {
        co_await consumer_->send_request(service_type, method_type, request, callback);
    }

    template <typename Request>
    [[REMEMBER_CO_AWAIT]]
    auto send_request(
        rpc::ServiceType service_type,
        rpc::MethodType method_type,
        const Request& request,
        rpc::RpcCallback&& callback) -> kosio::async::Task<void> {
        co_await consumer_->send_request(service_type, method_type, request, std::move(callback));
    }

private:
    RaftNodeInfo                      raft_node_info_;
    std::unique_ptr<rpc::RpcConsumer> consumer_;
};
} // namespace vosfs::raft::detail