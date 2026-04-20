#pragma once
#include "raftpb/raft.pb.h"
#include "detail/config.hpp"
#include "vosfs/raft/detail/rocksdb_engine.hpp"

namespace vosfs::raft {
class Persister {
    static constexpr std::string_view HARD_STATE_KEY = "raft/hard_state";
    static constexpr std::string_view RAFT_NODE_INFO_KEY = "raft/raft_node_info";
    static constexpr std::string_view RAFT_CLUSTER_INFO_KEY = "raft/raft_cluster_info";
    static constexpr std::string_view DATA_CLUSTER_INFO_KEY = "raft/data_cluster_info";
    static constexpr std::string_view SNAPSHOT_METADATA_KEY = "raft/snapshot_metadata";
    static constexpr std::string_view SNAPSHOT_KEY = "raft/snapshot";

private:
    explicit Persister(detail::RocksDBEngine engine)
        : engine_(std::move(engine)) {}

public:
    Persister(Persister&& other) noexcept = default;
    auto operator=(Persister&& other) noexcept -> Persister& = default;

public:
    static auto create(std::string_view data_dir) -> Result<Persister>;

public:
    /**
     * @brief 初始化持久层
     * @param raft_node_info 本地节点信息
     * @param raft_cluster_info Raft服务器集群信息
     * @param data_cluster_info 数据服务器集群信息
     * @warning 只能在首次启动集群时调用！！！
     * @warning 若初始化失败进程会直接崩溃
     */
    void init(
        const RaftNodeInfo& raft_node_info,
        const RaftClusterInfo& raft_cluster_info,
        const DataClusterInfo& data_cluster_info) const;

public:
    void save_hard_state(const HardState& hard_state) const;

    [[nodiscard]] auto load_hard_state() const -> Result<HardState>;

    void save_raft_node_info(const RaftNodeInfo& raft_node_info) const;

    [[nodiscard]] auto load_raft_node_info() const -> Result<RaftNodeInfo>;

    void save_raft_cluster_info(const RaftClusterInfo& raft_cluster_info) const;

    [[nodiscard]] auto load_raft_cluster_info() const -> Result<RaftClusterInfo>;

    void save_data_cluster_info(const DataClusterInfo& data_cluster_info) const;

    [[nodiscard]] auto load_data_cluster_info() const -> Result<DataClusterInfo>;

    void save_snapshot_metadata(const SnapshotMetadata& snapshot_metadata) const;

    [[nodiscard]] auto load_snapshot_metadata() const -> Result<SnapshotMetadata>;

    void save_snapshot(const Snapshot& snapshot) const;

    [[nodiscard]] auto load_snapshot() const -> Result<Snapshot>;

    void save_entry(const LogEntry& entry) const;

    void save_entries(const google::protobuf::RepeatedPtrField<LogEntry>& entries) const;

    [[nodiscard]] auto load_entries(uint64_t last_included_index) const -> Result<std::vector<LogEntry>>;

    auto truncate_entries(uint64_t start_index, uint64_t end_index) const -> void;

private:
    static auto get_entry_key(uint64_t index) -> std::string {
        return "raft/log/" + std::to_string(index);
    }

private:
    detail::RocksDBEngine engine_;
};
} // namespace vosfs::raft