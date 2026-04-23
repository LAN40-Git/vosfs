#pragma once
#include <ranges>
#include "peer.hpp"
#include "persister.hpp"

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
    [[REMEMBER_CO_AWAIT]]
    auto broadcast_request_vote_request(
        const RequestVoteRequest& request,
        const std::function<Task<void>(const vrpc::Status& status, const RequestVoteResponse& response)>& callback) -> Task<void> {
        for (auto& peer : peers_ | std::views::values) {
            co_await peer->send_request_vote_request(request, callback);
        }
    }

    [[REMEMBER_CO_AWAIT]]
    auto unicast_append_entries_request(
        uint64_t member_id,
        const AppendEntriesRequest& request,
        const std::function<Task<void>(const vrpc::Status& status, const AppendEntriesResponse& response)>& callback) -> Task<void> {
        co_await peers_[member_id]->send_append_entries_request(request, callback);
    }

    [[REMEMBER_CO_AWAIT]]
    auto unicast_install_snapshot_request(
        uint64_t member_id,
        const InstallSnapshotRequest& request,
        const std::function<Task<void>(const vrpc::Status& status, const InstallSnapshotResponse& response)>& callback) -> Task<void> {
        co_await peers_[member_id]->send_install_snapshot_request(request, callback);
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