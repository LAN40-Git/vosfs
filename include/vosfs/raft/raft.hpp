#pragma once
#include "vosfs/raft/internal/transport.hpp"

namespace vosfs::raft {
class RaftNode {
public:

private:
    detail::Transport transport_;
};
} // namespace vosfs::raft