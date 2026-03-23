#pragma once
#include <nlohmann/json.hpp>
#include "vosfs/raft/internal/peer.hpp"

namespace vosfs::raft::detail {
class RaftCluster {
    using PeerMap = std::unordered_map<uint64_t, Peer>;

private:
    RaftCluster(uint64_t cluster_id,
                uint64_t member_id,
                std::string&& name,
                std::string&& host,
                uint16_t port,
                PeerMap&& peers,
                std::string_view path,
                nlohmann::json&& json);

private:
    uint64_t              cluster_id_;
    uint64_t              member_id_;
    std::string           name_;
    std::string           host_;
    uint16_t              port_;
    kosio::sync::Mutex    mutex_; // protect peers_
    PeerMap               peers_;
    std::filesystem::path path_;
    nlohmann::json        json_;
};
} // namespace vosfs::raft::detail