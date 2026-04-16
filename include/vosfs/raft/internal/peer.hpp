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
    static auto create(const RaftNodeInfo& node_info) -> kosio::async::Task<Result<Peer>>;

public:
    [[REMEMBER_CO_AWAIT]]
    auto shutdown() const -> kosio::async::Task<void>;

public:
    [[REMEMBER_CO_AWAIT]]
    auto send_request(
        rpc::ServiceType service_type,
        rpc::MethodType method_type,
        std::string_view req_payload,
        const rpc::RpcCallback& callback) const -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto send_request(
        rpc::ServiceType service_type,
        rpc::MethodType method_type,
        std::string&& req_payload,
        rpc::RpcCallback&& callback) const -> kosio::async::Task<void>;

private:
    RaftNodeInfo                      raft_node_info_;
    std::unique_ptr<rpc::RpcConsumer> consumer_;
};
} // namespace vosfs::raft::detail