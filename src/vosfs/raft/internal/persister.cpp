#include "vosfs/raft/internal/persister.hpp"
#include <kosio/common/debug.hpp>

auto vosfs::raft::detail::Persister::create(std::string_view data_dir) -> Result<Persister> {
    rocksdb::Options db_options;
    db_options.create_if_missing = true;
    rocksdb::WriteOptions write_options;
    rocksdb::ReadOptions read_options;
    write_options.sync = true;

    auto db_path = std::filesystem::path(data_dir) / DB_DIR;
    auto ret = RocksDBEngine::create(db_options, write_options, read_options, db_path);
    if (!ret) {
        return std::unexpected{ret.error()};
    }
    return Persister{std::move(ret.value())};
}

auto vosfs::raft::detail::Persister::persist(
    std::string_view key,
    std::string_view value) const -> Result<void> {
    if (auto status = engine_.put(key, value); !status.ok()) {
        LOG_ERROR("persist failed : {}", status.ToString());
        return std::unexpected{make_error(Error::kPersistFailed)};
    }
    return Result<void>{};
}

auto vosfs::raft::detail::Persister::persist_batch(
    const std::vector<KV>& kvs) const -> Result<void> {
    rocksdb::WriteBatch write_batch;
    for (const auto& kv : kvs) {
        write_batch.Put(kv.key, kv.value);
    }
    if (auto status = engine_.batch_write(write_batch); !status.ok()) {
        LOG_ERROR("persist batch failed : {}", status.ToString());
        return std::unexpected{make_error(Error::kPersistFailed)};
    }
    return Result<void>{};
}

auto vosfs::raft::detail::Persister::recover(
    std::string_view key,
    std::string* value) const -> Result<void> {
    if (auto status = engine_.get(key, value); !status.ok()) {
        LOG_ERROR("recover failed : {}", status.ToString());
        return std::unexpected{make_error(Error::kRecoverFailed)};
    }
    return Result<void>{};
}

auto vosfs::raft::detail::Persister::recover_batch(
    const std::vector<rocksdb::Slice>& keys,
    std::vector<std::string>& values) const -> Result<void> {
    auto vec_status = engine_.multi_get(keys, values);
    for (const auto& status : vec_status) {
        if (!status.ok()) {
            LOG_ERROR("recover batch failed: {}", status.ToString());
            return std::unexpected{make_error(Error::kRecoverFailed)};
        }
    }

    return Result<void>{};
}

auto vosfs::raft::detail::Persister::truncate(
    std::string_view key) const -> Result<void> {
    if (auto status = engine_.truncate(key); !status.ok()) {
        LOG_ERROR("truncate failed : {}", status.ToString());
        return std::unexpected{make_error(Error::kTruncateFailed)};
    }
    return Result<void>{};
}

auto vosfs::raft::detail::Persister::truncate_batch(
    const std::vector<std::string>& keys) const -> Result<void> {
    rocksdb::WriteBatch write_batch;
    for (const auto& key : keys) {
        write_batch.Delete(key);
    }
    if (auto status = engine_.batch_write(write_batch); !status.ok()) {
        LOG_ERROR("truncate batch failed : {}", status.ToString());
        return std::unexpected{make_error(Error::kTruncateFailed)};
    }
    return Result<void>{};
}
