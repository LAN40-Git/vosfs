#pragma once
#include <filesystem>
#include <kosio/fs.hpp>
#include <kosio/sync.hpp>
#include "vosfs/common/error.hpp"
#include "raftpb/raft.pb.h"

namespace vosfs::raft::detail {
using kosio::fs::File;
using kosio::sync::Mutex;
using kosio::async::Task;
class Snapshotter {
public:
    using Ptr = std::unique_ptr<Snapshotter>;

    Snapshotter() = default;

    explicit Snapshotter(std::string_view tmp_path, std::string_view snapshot_path)
        : tmp_path_(tmp_path)
        , snapshot_path_(snapshot_path) {}

public:
    [[nodiscard]]
    auto offset_at(uint64_t member_id) -> Task<uint64_t>;

    void reset_offsets();

    [[nodiscard]]
    auto load_snapshot() -> Result<Snapshot>;

    [[REMEMBER_CO_AWAIT]]
    auto save_snapshot(Snapshot snapshot) -> Task<Result<void>>;

    [[REMEMBER_CO_AWAIT]]
    auto load_snapshot_data(uint64_t member_id, InstallSnapshotRequest& request) -> Task<Result<void>>;

    [[REMEMBER_CO_AWAIT]]
    auto save_snapshot_data(std::string data) -> Task<Result<void>>;

private:
    Mutex                                  mutex_; // TODO: 改用读写锁
    std::unordered_map<uint64_t, uint64_t> offsets_;
    uint64_t                               snapshot_size_{0};
    std::string                            tmp_path_;
    std::string                            snapshot_path_;
};
} // namespace vosfs::raft::detail