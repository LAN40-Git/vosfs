#pragma once
#include <cstdint>
#include "vosfs/rpc/consumer.hpp"
#include "vosfs/raft/internal/config.hpp"

namespace vosfs::raft::detail {
class Peer {
private:
    explicit Peer(uint64_t member_id, std::string_view name,
        std::string_view host, std::unique_ptr<rpc::RpcConsumer> consumer);

public:
    Peer(const Peer&) = delete;
    auto operator=(const Peer&) -> Peer& = delete;
    Peer(Peer&& other) noexcept = default;
    auto operator=(Peer&&) noexcept -> Peer& = default;

public:
    [[nodiscard]]
    auto member_id() const noexcept -> uint64_t { return member_id_; }
    [[nodiscard]]
    auto name() const noexcept -> std::string_view { return name_; }
    [[nodiscard]]
    auto host() const noexcept -> std::string_view { return host_; }

public:
    [[REMEMBER_CO_AWAIT]]
    static auto create(uint64_t member_id, std::string_view name, std::string_view host) -> kosio::async::Task<Result<Peer>>;

public:
    [[REMEMBER_CO_AWAIT]]
    auto shutdown() const -> kosio::async::Task<Result<void>>;

    [[REMEMBER_CO_AWAIT]]
    auto check_status() const -> kosio::async::Task<Result<void>>;

public:
    [[REMEMBER_CO_AWAIT]]
    auto send_request(
        rpc::ServiceType service_type,
        rpc::MethodType method_type,
        std::string_view req_payload,
        const rpc::RpcCallback& callback) const -> kosio::async::Task<Result<void>>;

    [[REMEMBER_CO_AWAIT]]
    auto send_request(
        rpc::ServiceType service_type,
        rpc::MethodType method_type,
        std::string&& req_payload,
        rpc::RpcCallback&& callback) const -> kosio::async::Task<Result<void>>;

private:
    uint64_t                          member_id_;
    std::string                       name_;
    std::string                       host_;
    // connection to this peer
    std::unique_ptr<rpc::RpcConsumer> consumer_;
};
} // namespace vosfs::raft::detail