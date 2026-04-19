#pragma once
#include <vrpc/core/type.hpp>

namespace vosfs::raft::detail {
enum class ServiceType : vrpc::Type {
    kRaft = 0,
    kFileSystem,
    kMaxService
};

enum class InvokeType : vrpc::Type {
    kRaftRequestVote = 0,
    kRaftAppendEntries,
    kRaftInstallSnapshot,
    kMaxMethod
};
} // namespace vosfs::raft::detail