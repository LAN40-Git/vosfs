#include <filesystem>
#include <nlohmann/json.hpp>
#include "vosfs/raft/transport.hpp"

vosfs::raft::detail::Transport::Transport(
    uint64_t cluster_id,
    uint64_t member_id,
    std::string name,
    std::string host,
    uint16_t port,
    PeerMap peers)
    : cluster_id_(cluster_id)
    , cluster_size_(peers.size() + 1)
    , member_id_(member_id)
    , name_(std::move(name))
    , host_(std::move(host))
    , port_(port)
    , peers_(std::move(peers)) {}

auto vosfs::raft::detail::Transport::create(const Config& config) -> Result<Transport> {
    // 集群配置文件
    auto config_file = std::filesystem::path(config.config_path);

    std::ifstream f(config_file);
    if (!f.is_open()) {
        return std::unexpected(make_error(Error::kFileOpenFailed));
    }
    auto json = nlohmann::json::parse(f);
    auto cluster_id = json["cluster_id"].get<uint64_t>();
    auto member_id = json["member_id"].get<uint64_t>();
    auto name = json["name"].get<std::string>();
    auto host = json["host"].get<std::string>();
    auto port = json["port"].get<uint16_t>();
    auto& nodes = json["nodes"];
    PeerMap peers;
    for (const auto& node : nodes) {
        auto node_id = node["id"].get<uint64_t>();
        auto node_name = node["name"].get<std::string>();
        auto node_host = node["host"].get<std::string>();
        auto node_port = node["port"].get<uint16_t>();
        if (node_id == member_id) {
            continue;
        }

        peers.emplace(node_id, std::make_shared<Peer>(node_id, node_name, node_host, node_port));
    }
    return Transport{cluster_id, member_id, name, host, port, std::move(peers)};
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

