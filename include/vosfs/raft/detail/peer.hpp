#pragma once
#include <vrpc/client.hpp>
#include "vosfs/raft/detail/rpc.hpp"
#include "vosfs/api/serverpb/raft.pb.h"
#include "vosfs/raft/detail/config.hpp"

namespace vosfs::raft::detail {
class Peer {
public:
    explicit Peer(uint64_t id, std::string_view name, std::string_view peer_ip)
        : id_(id)
        , name_(name)
        , client_(peer_ip, RAFT_RPC_PORT) {}

public:
    Peer(const Peer&) = delete;
    auto operator=(const Peer&) -> Peer& = delete;
    Peer(Peer&& other) noexcept = delete;
    auto operator=(Peer&&) noexcept -> Peer& = delete;

public:
    [[nodiscard]]
    auto id() const noexcept -> uint64_t { return id_; }
    [[nodiscard]]
    auto name() const noexcept -> std::string_view { return name_; }
    [[nodiscard]]
    auto ip() const noexcept -> std::string_view { return client_.server_ip(); }
    [[nodiscard]]
    auto port() const noexcept -> uint16_t { return client_.server_port(); }

public:
    [[REMEMBER_CO_AWAIT]]
    auto shutdown() -> kosio::async::Task<void> {
        co_await client_.shutdown();
    }

public:
    template <vrpc::Proto P>
    [[REMEMBER_CO_AWAIT]]
    auto call(
        ServiceType service_type,
        InvokeType invoke_type,
        const P& request,
        const vrpc::Callback& callback) -> kosio::async::Task<void> {
        co_await client_.call(service_type, invoke_type, request, callback);
    }

private:
    uint64_t     id_;
    std::string  name_;
    vrpc::Client client_;
};
} // namespace vosfs::raft::detail