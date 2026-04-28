#pragma once
#include <string>
#include <fstream>
#include <kosio/common/debug.hpp>
#include <nlohmann/json.hpp>
#include "vosfs/common/error.hpp"

namespace vosfs::raft {
namespace detail {
static constexpr std::size_t MAX_SNAPSHOT_CHUNK_SIZE = 4 * 1024;
static constexpr std::size_t HEARTBEAT_INTERVAL = 50;
static constexpr std::size_t MAX_APPEND_ENTRIES_SIZE = 64;
static constexpr std::size_t SNAPSHOT_INTERVAL = 10;
} // namespace detail

struct NodeInfo {
    uint64_t    id;
    std::string name;
    std::string host;
    uint16_t    port;
};

struct Config {
    static auto from_json(const std::string& path) -> Result<Config> {
        std::ifstream f(path);
        if (!f.is_open()) {
            return std::unexpected(make_error(Error::kFileOpenFailed));
        }

        auto json = nlohmann::json::parse(f);
        auto data_dir = json["data_dir"].get<std::string>();
        auto cluster_id = json["cluster_id"].get<uint64_t>();
        auto local_id = json["member_id"].get<uint64_t>();
        auto local_name = json["name"].get<std::string>();
        auto local_host = json["host"].get<std::string>();
        auto local_port = json["port"].get<uint16_t>();
        std::unordered_map<uint64_t, NodeInfo> nodes;

        auto& nodes_json = json["nodes"];
        for (auto& node : nodes_json) {
            auto id = node["id"].get<uint64_t>();
            auto name = node["name"].get<std::string>();
            auto host = node["host"].get<std::string>();
            auto port = node["port"].get<uint16_t>();
            nodes.emplace(id, NodeInfo{id, name, host, port});
        }

        std::cout << json.dump(4) << std::endl;

        return Config{
            .data_dir = std::move(data_dir),
            .cluster_id = cluster_id,
            .member_id = local_id,
            .name = std::move(local_name),
            .host = std::move(local_host),
            .port = local_port,
            .nodes = std::move(nodes)
        };
    }

    void to_json(const std::string& path) const {
        nlohmann::json json;
        json["data_dir"] = data_dir;
        json["cluster_id"] = cluster_id;
        json["member_id"] = member_id;
        json["name"] = name;
        json["host"] = host;
        json["port"] = port;

        nlohmann::json nodes_json;
        for (const auto& node : nodes | std::views::values) {
            nlohmann::json node_json;
            node_json["id"] = node.id;
            node_json["name"] = node.name;
            node_json["host"] = node.host;
            node_json["port"] = node.port;
            nodes_json.push_back(std::move(node_json));
        }
        json["nodes"] = std::move(nodes_json);

        std::ofstream f(path);
        f << json.dump(4);
    }

    // vosfs 数据目录
    std::string data_dir{"/var/lib/vosfs"};

    // 集群 id
    uint64_t cluster_id{0};

    // 节点 id
    uint64_t member_id{0};

    // 节点名称
    std::string name{"node"};

    // 监听 host
    std::string host{"0.0.0.0"};

    // 监听端口
    uint16_t port{8080};

    // 集群节点
    std::unordered_map<uint64_t, NodeInfo> nodes;
};

struct ConfigBuilder {
    [[nodiscard]]
    auto set_data_dir(std::string data_dir) -> ConfigBuilder& {
        config_.data_dir = std::move(data_dir);
        return *this;
    }

    [[nodiscard]]
    auto set_cluster_id(uint64_t cluster_id) -> ConfigBuilder& {
        config_.cluster_id = cluster_id;
        return *this;
    }

    [[nodiscard]]
    auto set_member_id(uint64_t member_id) -> ConfigBuilder& {
        config_.member_id = member_id;
        return *this;
    }

    [[nodiscard]]
    auto set_name(std::string name) -> ConfigBuilder& {
        config_.name = std::move(name);
        return *this;
    }

    [[nodiscard]]
    auto set_host(std::string host) -> ConfigBuilder& {
        config_.host = std::move(host);
        return *this;
    }

    [[nodiscard]]
    auto set_port(uint16_t port) -> ConfigBuilder& {
        config_.port = port;
        return *this;
    }

    [[nodiscard]]
    auto add_node(uint64_t id, std::string name, std::string host, uint16_t port) -> ConfigBuilder& {
        config_.nodes.emplace(id, NodeInfo{id, std::move(name), std::move(host), port});
        return *this;
    }

    [[nodiscard]]
    auto build() const -> Config {
        return config_;
    }

public:
    static auto options() -> ConfigBuilder {
        return ConfigBuilder{};
    }

private:
    Config config_;
};
} // namespace vosfs::raft