#include "vosfs/raft/internal/cluster.hpp"

vosfs::raft::detail::RaftCluster::RaftCluster(
    uint64_t cluster_id,
    uint64_t member_id,
    std::string&& name,
    std::string&& host,
    uint16_t port,
    PeerMap&& peers,
    std::string_view path,
    nlohmann::json&& json)
    : cluster_id_(cluster_id)
    , member_id_(member_id)
    , name_(std::move(name))
    , host_(std::move(host))
    , port_(port)
    , peers_(std::move(peers))
    , path_(path)
    , json_(std::move(json)) {}
