#pragma once
#include <cstdint>
#include "vosfs/rpc/consumer.hpp"

namespace vosfs::raft::detail {
class Peer {
public:
    explicit Peer(uint64_t member_id, std::string_view name,
        std::string_view host, uint16_t port,
        std::unique_ptr<rpc::RpcConsumer> consumer);

public:
    Peer(const Peer&) = delete;
    auto operator=(const Peer&) -> Peer& = delete;

public:
    [[nodiscard]]
    auto member_id() const noexcept -> uint64_t { return member_id_; }
    [[nodiscard]]
    auto name() const noexcept -> std::string_view { return name_; }
    [[nodiscard]]
    auto host() const noexcept -> std::string_view { return host_; }
    [[nodiscard]]
    auto port() const noexcept -> uint16_t { return port_; }

public:
    [[REMEMBER_CO_AWAIT]]
    auto shutdown() const -> kosio::async::Task<Result<void>>;

private:
    auto send_request(
        rpc::ServiceType service_type,
        rpc::MethodType method_type,
        std::string_view req_payload,
        const rpc::RpcCallback& callback) const -> kosio::async::Task<Result<void>>;

    auto send_request(
        rpc::ServiceType service_type,
        rpc::MethodType method_type,
        std::string&& req_payload,
        rpc::RpcCallback&& callback) const -> kosio::async::Task<Result<void>>;

private:
    uint64_t                          member_id_;
    std::string                       name_;
    std::string                       host_;
    uint16_t                          port_;
    std::unique_ptr<rpc::RpcConsumer> consumer_;
};
} // namespace vosfs::raft::detail