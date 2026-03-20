#pragma once
#include "vosfs/raft/internal/peer.hpp"

namespace vosfs::raft::detail {
class RaftCluster {
public:

private:
    uint64_t    cluster_id_;
    uint64_t    member_id_;
    std::string name_;

};
} // namespace vosfs::raft::detail