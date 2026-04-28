#include <filesystem>
#include <nlohmann/json.hpp>
#include "vosfs/raft/transport.hpp"

vosfs::raft::detail::Transport::Transport(const Config& config) {
    cluster_id_ = config.cluster_id;
    member_id_ = config.member_id;
    name_ = config.name;
    host_ = config.host;
    port_ = config.port;

    for (auto& node : config.nodes | std::views::values) {
        auto peer = std::make_shared<Peer>(node.id, node.name, node.host, node.port);
        peers_.emplace(node.id, std::move(peer));
    }
}

auto vosfs::raft::detail::Transport::shutdown() const -> Task<void> {
    for (const auto& peer : peers_ | std::views::values) {
        co_await peer->shutdown();
    }
}

auto vosfs::raft::detail::Transport::broadcast_request_vote_request(
    const RequestVoteRequest& request,
    const std::function<Task<void>(
        const vrpc::Status& status,
        const RequestVoteResponse& response)>& callback) -> Task<void> {
    for (auto& peer : peers_ | std::views::values) {
        if (peer->id() == member_id_) {
            continue;
        }
        co_await peer->send_request_vote_request(request, callback);
    }
}

auto vosfs::raft::detail::Transport::unicast_append_entries_request(uint64_t member_id,
    const AppendEntriesRequest& request,
    const std::function<Task<void>(
        const vrpc::Status& status,
        const AppendEntriesResponse& response)>& callback) -> Task<void> {
    co_await peers_[member_id]->send_append_entries_request(request, callback);
}

auto vosfs::raft::detail::Transport::unicast_install_snapshot_request(uint64_t member_id,
    const InstallSnapshotRequest& request,
    const std::function<Task<void>(
        const vrpc::Status& status,
        const InstallSnapshotResponse& response)>& callback) -> Task<void> {
    co_await peers_[member_id]->send_install_snapshot_request(request, callback);
}

