#include "vosfs/raft/persister.hpp"
#include <kosio/common/debug.hpp>

auto vosfs::raft::Persister::create(std::string_view data_dir) -> Result<Persister> {
    rocksdb::Options db_options;
    db_options.create_if_missing = true;
    rocksdb::WriteOptions write_options;
    rocksdb::ReadOptions read_options;
    write_options.sync = true;

    auto db_path = std::filesystem::path(data_dir) / detail::DB_DIR;
    auto ret = detail::RocksDBEngine::create(db_options, write_options, read_options, db_path);
    if (!ret) {
        return std::unexpected{ret.error()};
    }
    return Persister{std::move(ret.value())};
}

void vosfs::raft::Persister::save_hard_state(const HardState& hard_state) const {
    if (auto status = engine_.put(HARD_STATE_KEY, hard_state.SerializeAsString()); !status.ok()) {
        LOG_FATAL("failed to save hard state: {}", status.ToString());
        std::abort();
    }
}

auto vosfs::raft::Persister::load_hard_state() const -> Result<HardState> {
    std::string payload;
    HardState hard_state;
    if (auto status = engine_.get(HARD_STATE_KEY, &payload); !status.ok()) {
        LOG_ERROR("failed to load hard state: {}", status.ToString());
        return std::unexpected{make_error(Error::kRecoverFailed)};
    }
    if (!hard_state.ParseFromString(payload)) {
        return std::unexpected{make_error(Error::kProtoParseFailed)};
    }
    return hard_state;
}

void vosfs::raft::Persister::save_raft_cluster_info(const RaftClusterInfo& raft_cluster_info) const {
    if (auto status = engine_.put(RAFT_CLUSTER_INFO_KEY, raft_cluster_info.SerializeAsString()); !status.ok()) {
        LOG_FATAL("failed to save raft cluster info : {}", status.ToString());
        std::abort();
    }
}

auto vosfs::raft::Persister::load_raft_cluster_info() const -> Result<RaftClusterInfo> {
    std::string payload;
    RaftClusterInfo raft_cluster_info;
    if (auto status = engine_.get(RAFT_CLUSTER_INFO_KEY, &payload); !status.ok()) {
        LOG_ERROR("failed to load raft cluster info : {}", status.ToString());
        return std::unexpected{make_error(Error::kRecoverFailed)};
    }
    if (!raft_cluster_info.ParseFromString(payload)) {
        return std::unexpected{make_error(Error::kProtoParseFailed)};
    }
    return raft_cluster_info;
}

void vosfs::raft::Persister::save_raft_node_info(const RaftNodeInfo& raft_node_info) const {
    if (auto status = engine_.put(RAFT_NODE_INFO_KEY, raft_node_info.SerializeAsString()); !status.ok()) {
        LOG_FATAL("failed to save raft node info : {}", status.ToString());
        std::abort();
    }
}

auto vosfs::raft::Persister::load_raft_node_info() const -> Result<RaftNodeInfo> {
    std::string payload;
    RaftNodeInfo raft_node_info;
    if (auto status = engine_.get(RAFT_NODE_INFO_KEY, &payload); !status.ok()) {
        LOG_ERROR("failed to load raft node info : {}", status.ToString());
        return std::unexpected{make_error(Error::kRecoverFailed)};
    }
    if (!raft_node_info.ParseFromString(payload)) {
        return std::unexpected{make_error(Error::kProtoParseFailed)};
    }
    return raft_node_info;
}

void vosfs::raft::Persister::save_data_cluster_info(const DataClusterInfo& data_cluster_info) const {
    if (auto status = engine_.put(DATA_CLUSTER_INFO_KEY, data_cluster_info.SerializeAsString()); !status.ok()) {
        LOG_FATAL("failed to save data cluster info : {}", status.ToString());
        std::abort();
    }
}

auto vosfs::raft::Persister::load_data_cluster_info() const -> Result<DataClusterInfo> {
    std::string payload;
    DataClusterInfo data_cluster_info;
    if (auto status = engine_.get(DATA_CLUSTER_INFO_KEY, &payload); !status.ok()) {
        LOG_ERROR("failed to load data cluster info : {}", status.ToString());
        return std::unexpected{make_error(Error::kRecoverFailed)};
    }
    if (!data_cluster_info.ParseFromString(payload)) {
        return std::unexpected{make_error(Error::kProtoParseFailed)};
    }
    return data_cluster_info;
}

void vosfs::raft::Persister::save_snapshot_metadata(const SnapshotMetadata& snapshot_metadata) const {
    if (auto status = engine_.put(SNAPSHOT_METADATA_KEY, snapshot_metadata.SerializeAsString()); !status.ok()) {
        LOG_FATAL("failed to save snapshot metadata : {}", status.ToString());
        std::abort();
    }
}

auto vosfs::raft::Persister::load_snapshot_metadata() const -> Result<SnapshotMetadata> {
    std::string payload;
    SnapshotMetadata snapshot_metadata;
    if (auto status = engine_.get(SNAPSHOT_METADATA_KEY, &payload); !status.ok()) {
        LOG_ERROR("failed to load snapshot metadata : {}", status.ToString());
        return std::unexpected{make_error(Error::kRecoverFailed)};
    }
    if (!snapshot_metadata.ParseFromString(payload)) {
        return std::unexpected{make_error(Error::kProtoParseFailed)};
    }
    return snapshot_metadata;
}

void vosfs::raft::Persister::save_snapshot(const std::string& snapshot_data) const {
    if (auto status = engine_.put(SNAPSHOT_KEY, snapshot_data); !status.ok()) {
        LOG_FATAL("failed to save snapshot : {}", status.ToString());
        std::abort();
    }
}

auto vosfs::raft::Persister::load_snapshot() const -> Result<std::string> {
    std::string payload;
    if (auto status = engine_.get(SNAPSHOT_KEY, &payload); !status.ok()) {
        LOG_ERROR("failed to load snapshot : {}", status.ToString());
        return std::unexpected{make_error(Error::kRecoverFailed)};
    }
    return payload;
}

void vosfs::raft::Persister::save_entry(const LogEntry& entry) const {
    if (auto status = engine_.put(get_entry_key(entry.index()), entry.SerializeAsString()); !status.ok()) {
        LOG_FATAL("failed to save entry at {} : {}", entry.index(), status.ToString());
        std::abort();
    }
}

void vosfs::raft::Persister::save_entries(const google::protobuf::RepeatedPtrField<LogEntry>& entries) const {
    rocksdb::WriteBatch write_batch;
    for (const auto& entry : entries) {
        write_batch.Put(get_entry_key(entry.index()), entry.SerializeAsString());
    }

    if (auto status = engine_.batch_write(write_batch); !status.ok()) {
        LOG_FATAL("failed to save entries : {}", status.ToString());
        std::abort();
    }
}

auto vosfs::raft::Persister::load_entries(uint64_t last_included_index) const -> Result<std::vector<LogEntry>> {
    auto index = last_included_index + 1;
    std::vector<LogEntry> entries;
    while (true) {
        std::string payload;
        LogEntry entry;
        if (auto status = engine_.get(get_entry_key(index++), &payload); !status.ok()) {
            if (status.code() != rocksdb::Status::kNotFound) {
                LOG_ERROR("failed to load entries : {}", status.ToString());
                return std::unexpected{make_error(Error::kRecoverFailed)};
            }
            break;
        }

        if (!entry.ParseFromString(payload)) {
            return std::unexpected{make_error(Error::kProtoParseFailed)};
        }
        entries.emplace_back(std::move(entry));
    }
    return entries;
}

void vosfs::raft::Persister::truncate_entries(uint64_t start_index, uint64_t end_index) const {
    rocksdb::WriteBatch write_batch;
    for (uint64_t index = start_index; index <= end_index; ++index) {
        write_batch.Delete(get_entry_key(index));
    }

    if (auto status = engine_.batch_write(write_batch); !status.ok()) {
        LOG_FATAL("failed to truncate entries from {} to {} : {}", start_index, end_index, status.ToString());
        std::abort();
    }
}
