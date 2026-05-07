#pragma once
#include <string>
#include <fstream>
#include <kosio/common/debug.hpp>
#include <nlohmann/json.hpp>
#include "vosfs/common/error.hpp"
#include "vosfs/common/util/file.hpp"

namespace vosfs::ui {
namespace detail {
struct NodeInfo {
    uint64_t    id;
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
        auto auth_host = json["auth_host"].get<std::string>();
        auto auth_port = json["auth_port"].get<uint16_t>();
        std::unordered_map<uint64_t, NodeInfo> nodes;

        auto& nodes_json = json["nodes"];
        for (auto& node : nodes_json) {
            auto id = node["id"].get<uint64_t>();
            auto host = node["host"].get<std::string>();
            auto port = node["port"].get<uint16_t>();
            nodes.emplace(id, NodeInfo{id, host, port});
        }

        std::cout << json.dump(4) << std::endl;

        return Config{
            .auth_host = std::move(auth_host),
            .auth_port = auth_port,
            .nodes = std::move(nodes)
        };
    }

    void to_json(const std::string& path) const {
        if (std::filesystem::exists(path)) {
            return;
        }

        nlohmann::json json;
        json["auth_host"] = auth_host;
        json["auth_port"] = auth_port;

        nlohmann::json nodes_json;
        for (const auto& node : nodes | std::views::values) {
            nlohmann::json node_json;
            node_json["id"] = node.id;
            node_json["host"] = node.host;
            node_json["port"] = node.port;
            nodes_json.push_back(std::move(node_json));
        }
        json["nodes"] = std::move(nodes_json);

        std::ofstream f(path);
        f << json.dump(4);
    }

    // 认证服务器 host
    std::string auth_host{"127.0.0.1"};

    // 认证服务器端口
    uint16_t auth_port{9000};

    // 集群节点
    std::unordered_map<uint64_t, NodeInfo> nodes;
};
} // namespace detail

struct ConfigBuilder {
    [[nodiscard]]
    auto set_auth_host(std::string auth_host) -> ConfigBuilder& {
        config_.auth_host = std::move(auth_host);
        return *this;
    }

    [[nodiscard]]
    auto set_auth_port(uint16_t auth_port) -> ConfigBuilder& {
        config_.auth_port = auth_port;
        return *this;
    }

    [[nodiscard]]
    auto add_node(uint64_t id, std::string host, uint16_t port) -> ConfigBuilder& {
        config_.nodes.emplace(id, detail::NodeInfo{id,  std::move(host), port});
        return *this;
    }

    [[nodiscard]]
    auto build() const -> detail::Config {
        return config_;
    }

public:
    static auto options() -> ConfigBuilder {
        return ConfigBuilder{};
    }

private:
    detail::Config config_;
};
} // namespace vosfs::ui