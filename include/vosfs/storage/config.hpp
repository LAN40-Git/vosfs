#pragma once
#include <cstdint>
#include <string>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include "vosfs/common/error.hpp"

namespace vosfs::storage {
namespace detail {
struct Config {
    static auto from_json(const std::string& path) -> Result<Config> {
        std::ifstream f(path);
        if (!f.is_open()) {
            return std::unexpected(make_error(Error::kFileOpenFailed));
        }

        auto json = nlohmann::json::parse(f);
        auto host = json["host"].get<std::string>();
        auto port = json["port"].get<uint16_t>();
        auto block_dir = json["block_dir"].get<std::string>();

        std::cout << json.dump(4) << std::endl;

        return Config{
            .host = std::move(host),
            .port = port,
            .block_dir = std::move(block_dir),
        };
    }

    void to_json(const std::string& path) const {
        nlohmann::json json;
        json["host"] = host;
        json["port"] = port;
        json["block_dir"] = block_dir;
        std::ofstream f(path);
        f << json.dump(4);
    }

    std::string host{"0.0.0.0"};

    uint16_t port{7070};

    std::string block_dir{"vosfs_blocks"};
};
} // namespace detail

struct ConfigBuilder {
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
    auto set_block_dir(std::string block_dir) -> ConfigBuilder& {
        config_.block_dir = std::move(block_dir);
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
} // namespace vosfs::storage