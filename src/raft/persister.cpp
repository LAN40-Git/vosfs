#include "vosfs/raft/persister.hpp"
#include <kosio/common/debug.hpp>

auto vosfs::raft::Persister::create(std::string_view data_dir) -> Result<Persister> {
    rocksdb::Options db_options;
    db_options.create_if_missing = true;
    rocksdb::WriteOptions write_options;
    rocksdb::ReadOptions read_options;
    write_options.sync = true;

    auto db_path = std::filesystem::path(data_dir) / detail::RAFT_DB_PATH;
    auto ret = util::RocksDBEngine::create(db_options, write_options, read_options, db_path);
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
        return std::unexpected{make_error(Error::kDatabaseQueryFailed)};
    }
    if (!hard_state.ParseFromString(payload)) {
        return std::unexpected{make_error(Error::kDatabaseQueryFailed)};
    }
    return hard_state;
}

void vosfs::raft::Persister::save_entry(const LogEntry& entry) const {
    if (auto status = engine_.put(get_entry_key(entry.index()), entry.SerializeAsString()); !status.ok()) {
        LOG_FATAL("failed to save entry at {}: {}", entry.index(), status.ToString());
        std::abort();
    }
}

void vosfs::raft::Persister::save_entries(const google::protobuf::RepeatedPtrField<LogEntry>& entries) const {
    rocksdb::WriteBatch write_batch;
    for (const auto& entry : entries) {
        write_batch.Put(get_entry_key(entry.index()), entry.SerializeAsString());
    }

    if (auto status = engine_.batch_write(write_batch); !status.ok()) {
        LOG_FATAL("failed to save entries: {}", status.ToString());
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
                LOG_ERROR("failed to load entries: {}", status.ToString());
                return std::unexpected{make_error(Error::kDatabaseQueryFailed)};
            }
            break;
        }

        if (!entry.ParseFromString(payload)) {
            return std::unexpected{make_error(Error::kDatabaseQueryFailed)};
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
        LOG_FATAL("failed to truncate entries from {} to {}: {}", start_index, end_index, status.ToString());
        std::abort();
    }
}
