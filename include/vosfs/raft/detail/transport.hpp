#pragma once
#include <ranges>
#include "vosfs/raft/detail/peer.hpp"
#include "vosfs/raft/persister.hpp"

namespace vosfs::raft::detail {
class Transport {
    using PeerMap = std::unordered_map<uint64_t, std::unique_ptr<Peer>>;
public:
    explicit Transport(
        uint64_t member_id,
        std::string_view name,
        std::string_view ip,
        const RaftClusterInfo& cluster_info);

public:
    static auto create(const Persister& persister) -> kosio::async::Task<Result<Transport>>;

public:
    [[REMEMBER_CO_AWAIT]]
    auto shutdown() const -> kosio::async::Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto apply_snapshot(Snapshot& snapshot) -> kosio::async::Task<void>;

public:
    template <typename T>
    [[REMEMBER_CO_AWAIT]]
    auto unicast_request(
        uint64_t peer_id,
        ServiceType service_type,
        InvokeType invoke_type,
        const T& request,
        const vrpc::Callback& callback) -> kosio::async::Task<void> {
        auto it = peers_.find(peer_id);
        if (it == peers_.end()) {
            LOG_ERROR("peer {} not found", peer_id);
            co_return;
        }

        auto& peer = it->second;
        co_await peer->call(service_type, invoke_type, request, callback);
    }

    template <typename T>
    [[REMEMBER_CO_AWAIT]]
    auto broadcast_request(
        ServiceType service_type,
        InvokeType invoke_type,
        const T& request,
        const vrpc::Callback& callback) -> kosio::async::Task<void> {
        for (auto& peer : peers_ | std::views::values) {
            co_await peer->call(service_type, invoke_type, request, callback);
        }
    }

public:
    [[nodiscard]]
    auto cluster_id() const noexcept -> uint64_t { return cluster_id_; }
    [[nodiscard]]
    auto cluster_size() const noexcept -> uint64_t { return cluster_size_; }
    [[nodiscard]]
    auto member_id() const noexcept -> uint64_t { return member_id_; }
    [[nodiscard]]
    auto name() const noexcept -> std::string_view { return name_; }
    [[nodiscard]]
    auto ip() const noexcept -> std::string_view { return ip_; }
    [[nodiscard]]
    auto peers() -> PeerMap& { return peers_; }

private:
    uint64_t    cluster_id_;
    uint64_t    cluster_size_;
    uint64_t    member_id_;
    std::string name_;
    std::string ip_;
    PeerMap     peers_;
};
} // namespace vosfs::raft::detail