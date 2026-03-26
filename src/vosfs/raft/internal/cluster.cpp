#include "vosfs/raft/internal/cluster.hpp"

vosfs::raft::detail::RaftCluster::RaftCluster(
    uint64_t cluster_id,
    uint64_t member_id,
    std::string&& name,
    std::string&& host,
    PeerMap&& peers,
    std::string_view path,
    nlohmann::json&& json)
    : cluster_id_(cluster_id)
    , member_id_(member_id)
    , name_(std::move(name))
    , host_(std::move(host))
    , peers_(std::move(peers))
    , path_(path)
    , json_(std::move(json)) {}

auto vosfs::raft::detail::RaftCluster::create(
    std::string_view path, uint64_t cluster_id, std::string_view name,
    const std::unordered_set<NodeInfo>& node_infos) ->  kosio::async::Task<Result<void>> {
    std::filesystem::path config_file_path(path);
    if (!config_file_path.has_parent_path()) {
        std::filesystem::create_directories(config_file_path.parent_path());
    }
    auto has_config_file = co_await kosio::fs::File::options()
        .create(true)
        .read(true)
        .write(true)
        .truncate(true)
        .permission(0600)
        .open(path);
    if (!has_config_file) {
        LOG_ERROR("{}", has_config_file.error());
        co_return std::unexpected{make_error(Error::kRaftConfigFileOpenFailed)};
    }
    auto config_file = std::move(has_config_file.value());

    nlohmann::json json;
    std::optional<uint64_t> local_member_id{std::nullopt};
    std::optional<std::string> local_host{std::nullopt};

    nlohmann::json nodes_json = nlohmann::json::array();
    for (auto& node_info : node_infos) {
        // Check peer address
        auto has_addr = kosio::net::SocketAddr::parse(node_info.host, RAFT_PROVIDER_PORT);
        if (!has_addr) {
            LOG_ERROR("{}", has_addr.error());
            co_return std::unexpected{make_error(Error::kInvalidAddress)};
        }

        // Generate member_id
        auto member_id = node_info.hash();
        if (node_info.name == name) {
            local_member_id = member_id;
            local_host = node_info.host;
        }

        nlohmann::json node_entry;
        node_entry["member_id"] = member_id;
        node_entry["name"] = node_info.name;
        node_entry["host"] = node_info.host;
        nodes_json.push_back(node_entry);
    }

    if (!local_member_id.has_value() || !local_host.has_value()) {
        co_return std::unexpected{make_error(Error::kLocalNodeNotFound)};
    }

    json["cluster_id"] = cluster_id;
    json["member_id"] = local_member_id.value();
    json["name"] = name;
    json["host"] = local_host.value();
    json["nodes"] = nodes_json;

    auto config_payload = json.dump(4);

    if (auto ret = co_await config_file.write_all(config_payload); !ret) {
        co_return std::unexpected{make_error(Error::kRaftConfigFileWriteFailed)};
    }
    co_return Result<void>{};
}

auto vosfs::raft::detail::RaftCluster::load(std::string_view path) -> kosio::async::Task<Result<RaftCluster>> {
    std::filesystem::path config_file_path(path);
    std::ifstream config_file(config_file_path);

    if (!config_file) {
        co_return std::unexpected{make_error(errno)};
    }

    try {
        auto json = nlohmann::json::parse(config_file);
        config_file.close();

        auto cluster_id = json["cluster_id"].get<uint64_t>();
        auto local_member_id = json["member_id"].get<std::uint64_t>();
        auto local_name = json["name"].get<std::string>();
        auto local_host = json["host"].get<std::string>();
        auto has_node_addr = kosio::net::SocketAddr::parse(local_host, RAFT_PROVIDER_PORT);
        if (!has_node_addr) {
            LOG_ERROR("{}", has_node_addr.error());
            co_return std::unexpected{make_error(Error::kInvalidAddress)};
        }

        PeerMap peers;
        const auto& nodes_json = json["nodes"];
        for (const auto& node_json : nodes_json) {
            NodeInfo node_info;
            auto member_id = node_json["member_id"].get<uint64_t>();
            // ignore myself
            if (local_member_id == member_id) {
                continue;
            }
            node_info.name = node_json["name"].get<std::string>();
            node_info.host = node_json["host"].get<std::string>();

            // check repeat
            if (peers.contains(member_id)) {
                co_return std::unexpected{make_error(Error::kRepeatedPeer)};
            }

            auto has_peer = co_await Peer::create(member_id, node_info.name, node_info.host);
            if (!has_peer) {
                co_return std::unexpected{has_peer.error()};
            }
            peers.emplace(member_id, std::move(has_peer.value()));
        }

        co_return RaftCluster{
            cluster_id,
            local_member_id,
            std::move(local_name),
            std::move(local_host),
            std::move(peers),
            path,
            std::move(json)};
    } catch (const nlohmann::json::parse_error& e) {
        LOG_VERBOSE("Failed to parse json file", e.what());
        co_return std::unexpected{make_error(Error::kJsonParseFailed)};
    } catch (...) {
        LOG_VERBOSE("Unknown exception while loading config");
        co_return std::unexpected{make_error(Error::kJsonParseFailed)};
    }
}
