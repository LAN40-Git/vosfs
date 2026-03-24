#pragma once
#include <nlohmann/json.hpp>
#include <kosio/fs.hpp>
#include <xxh3.h>
#include "vosfs/raft/internal/config.hpp"
#include "vosfs/raft/internal/peer.hpp"

namespace vosfs::raft::detail {
class RaftCluster {
    using PeerMap = std::unordered_map<uint64_t, Peer>;

    struct NodeInfo {
        std::string name;
        std::string host;

        auto operator==(const NodeInfo& other) const noexcept -> bool {
            return name == other.name && host == other.host;
        }

        [[nodiscard]]
        auto hash() const noexcept -> uint64_t {
            XXH3_state_t state;
            XXH3_64bits_reset(&state);

            XXH3_64bits_update(&state, name.data(), name.size());
            XXH3_64bits_update(&state, host.data(), host.size());

            return XXH3_64bits_digest(&state);
        }
    };

private:
    RaftCluster(uint64_t cluster_id,
                uint64_t member_id,
                std::string&& name,
                std::string&& host,
                PeerMap&& peers,
                std::string_view path,
                nlohmann::json&& json);

public:
    static auto create(
        std::string_view path,
        uint64_t cluster_id,
        std::string_view name,
        const std::unordered_set<NodeInfo>& node_infos) -> kosio::async::Task<Result<void>>;

private:
    uint64_t              cluster_id_;
    uint64_t              member_id_;
    std::string           name_;
    std::string           host_;
    PeerMap               peers_;
    std::filesystem::path path_;
    nlohmann::json        json_;
};
} // namespace vosfs::raft::detail