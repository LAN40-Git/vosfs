#pragma once
#include <cstdint>
#include "vosfs/api/serverpb/raft.pb.h"
#include "vosfs/rpc/consumer.hpp"
#include "vosfs/raft/config.hpp"

namespace vosfs::raft::detail {
class Peer {
private:
    explicit Peer(const NodeInfo& node_info, std::unique_ptr<rpc::RpcConsumer> consumer)
        : node_info_(node_info), consumer_(std::move(consumer)) {}

public:
    Peer(const Peer&) = delete;
    auto operator=(const Peer&) -> Peer& = delete;
    Peer(Peer&& other) noexcept = default;
    auto operator=(Peer&&) noexcept -> Peer& = default;

public:
    [[nodiscard]]
    auto member_id() const noexcept -> uint64_t { return node_info_.id(); }
    [[nodiscard]]
    auto name() const noexcept -> std::string_view { return node_info_.name(); }
    [[nodiscard]]
    auto host() const noexcept -> std::string_view { return node_info_.host(); }

public:
    [[REMEMBER_CO_AWAIT]]
    static auto create(const NodeInfo& node_info) -> kosio::async::Task<Result<Peer>>;

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
    NodeInfo node_info_;
    // connection to this peer
    std::unique_ptr<rpc::RpcConsumer> consumer_;
};
} // namespace vosfs::raft::detail