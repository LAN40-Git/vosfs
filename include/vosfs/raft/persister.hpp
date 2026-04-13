#pragma once
#include "raft.pb.h"
#include "vosfs/raft/config.hpp"
#include "vosfs/raft/internal/rocksdb_engine.hpp"

namespace vosfs::raft {
class Persister {
    static constexpr std::string_view HARD_STATE_KEY = "raft/hard_state";
    static constexpr std::string_view NODE_INFO_KEY = "raft/node_info";
    static constexpr std::string_view CLUSTER_INFO_KEY = "raft/cluster_info";
    static constexpr std::string_view SNAPSHOT_METADATA_KEY = "raft/snapshots/metadata";

private:
    explicit Persister(detail::RocksDBEngine engine)
        : engine_(std::move(engine)) {}

public:
    Persister(Persister&& other) noexcept = default;
    auto operator=(Persister&& other) noexcept -> Persister& = default;

public:
    static auto create(std::string_view data_dir) -> Result<Persister>;

public:
    [[nodiscard]]
    auto save_hard_state(const HardState& hard_state) const -> Result<void>;

    [[nodiscard]]
    auto load_hard_state() const -> Result<HardState>;

    [[nodiscard]]
    auto save_cluster_info(const ClusterInfo& cluster_info) const -> Result<void>;

    [[nodiscard]]
    auto load_cluster_info() const -> Result<ClusterInfo>;

    [[nodiscard]]
    auto save_node_info(const NodeInfo& node_info) const -> Result<void>;

    [[nodiscard]]
    auto load_node_info() const -> Result<NodeInfo>;

    [[nodiscard]]
    auto save_snapshot_metadata(const SnapshotMetadata& snapshot_metadata) const -> Result<void>;

    [[nodiscard]]
    auto load_snapshot_metadata() const -> Result<SnapshotMetadata>;

    [[nodiscard]]
    auto save_entry(const LogEntry& entry) const -> Result<void>;

    [[nodiscard]]
    auto save_entries(const google::protobuf::RepeatedPtrField<LogEntry>& entries) const -> Result<void>;

    [[nodiscard]]
    auto load_entries(uint64_t last_included_index) const -> Result<std::vector<LogEntry>>;

    [[nodiscard]]
    auto truncate_entries(uint64_t start_index, uint64_t end_index) const -> Result<void>;

private:
    static auto get_entry_key(uint64_t index) -> std::string {
        return "raft/log/" + std::to_string(index);
    }

private:
    detail::RocksDBEngine engine_;
};
} // namespace vosfs::raft