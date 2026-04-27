#pragma once
#include <filesystem>
#include <kosio/fs.hpp>
#include <kosio/sync.hpp>
#include "vosfs/common/error.hpp"
#include "raftpb/raft.pb.h"

namespace vosfs::raft {
using kosio::fs::File;
using kosio::sync::Mutex;
using kosio::async::Task;
class Snapshotter {
public:
    explicit Snapshotter(const std::filesystem::path& snapshot_dir)
        : snapshot_dir_(snapshot_dir) {}

public:
    auto load_snapshot() -> Task<Result<Snapshot>>;
    auto load_snapshotdata(uint64_t offset, std::size_t size) -> Task<Result<std::string>>;

private:
    Mutex                 mutex_;
    std::filesystem::path snapshot_dir_;
};
} // namespace vosfs::raft