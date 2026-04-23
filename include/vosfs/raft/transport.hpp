#pragma once
#include "peer.hpp"
#include "persister.hpp"

namespace vosfs::raft::detail {
class Transport {
    using PeerMap = std::unordered_map<uint64_t, std::shared_ptr<Peer>>;
public:
    explicit Transport(
        uint64_t cluster_id,
        uint64_t member_id,
        std::string name,
        std::string host,
        uint16_t port,
        PeerMap peers);

public:
    static auto create(const Config& config) -> Result<Transport>;

public:
    [[REMEMBER_CO_AWAIT]]
    auto shutdown() const -> Task<void>;

public:
    [[REMEMBER_CO_AWAIT]]
    auto broadcast_request_vote_request(
        const RequestVoteRequest& request,
        const std::function<Task<void>(
            const vrpc::Status& status,
            const RequestVoteResponse& response)>& callback) -> Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto unicast_append_entries_request(
        uint64_t member_id,
        const AppendEntriesRequest& request,
        const std::function<Task<void>(
            const vrpc::Status& status,
            const AppendEntriesResponse& response)>& callback) -> Task<void>;

    [[REMEMBER_CO_AWAIT]]
    auto unicast_install_snapshot_request(
        uint64_t member_id,
        const InstallSnapshotRequest& request,
        const std::function<Task<void>(
            const vrpc::Status& status,
            const InstallSnapshotResponse& response)>& callback) -> Task<void>;

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
    auto peers() -> PeerMap& { return peers_; }

private:
    uint64_t    cluster_id_;
    uint64_t    cluster_size_;
    uint64_t    member_id_;
    std::string name_;
    std::string host_;
    uint16_t    port_;
    PeerMap     peers_;
};
} // namespace vosfs::raft::detail