#pragma once
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
    explicit Snapshotter(std::string_view data_dir)
        : data_dir_(data_dir) {}

public:
    auto load_snapshot() -> Task<Result<Snapshot>>;

private:
    Mutex       mutex_;
    std::string snapshot_path_;
};
} // namespace vosfs::raft