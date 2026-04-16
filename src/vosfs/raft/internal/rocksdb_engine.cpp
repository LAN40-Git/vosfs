#include "vosfs/raft/internal/rocksdb_engine.hpp"
#include <kosio/common/debug.hpp>

vosfs::raft::detail::RocksDBEngine::~RocksDBEngine() {
    if (db_) {
        db_->Close();
        delete db_;
        db_ = nullptr;
    }

    if (checkpoint_) {
        delete checkpoint_;
        checkpoint_ = nullptr;
    }
}

vosfs::raft::detail::RocksDBEngine::RocksDBEngine(RocksDBEngine&& other) noexcept
    : db_(other.db_)
    , checkpoint_(other.checkpoint_)
    , write_options_(other.write_options_)
    , read_options_(other.read_options_) {
    other.db_ = nullptr;
    other.checkpoint_ = nullptr;
}

auto vosfs::raft::detail::RocksDBEngine::operator=(RocksDBEngine&& other) noexcept -> RocksDBEngine& {
    if (this == &other) {
        return *this;
    }

    if (db_) {
        db_->Close();
        delete db_;
        db_ = nullptr;
    }

    if (checkpoint_) {
        delete checkpoint_;
        checkpoint_ = nullptr;
    }

    db_ = other.db_;
    checkpoint_ = other.checkpoint_;
    write_options_ = other.write_options_;
    read_options_  = other.read_options_;
    other.db_ = nullptr;
    other.checkpoint_ = nullptr;
    return *this;
}

auto vosfs::raft::detail::RocksDBEngine::create(
    const rocksdb::Options& db_options,
    const rocksdb::WriteOptions& write_options,
    const rocksdb::ReadOptions& read_options,
    const std::filesystem::path& db_path) -> Result<RocksDBEngine> {
    rocksdb::DB* db = nullptr;
    rocksdb::Checkpoint* checkpoint = nullptr;
    auto status = rocksdb::DB::Open(db_options, db_path, &db);

    if (!status.ok()) {
        LOG_ERROR("{}", status.ToString());
        return std::unexpected{make_error(Error::kCreateRocksDBEngineFailed)};
    }

    status = rocksdb::Checkpoint::Create(db, &checkpoint);

    if (!status.ok()) {
        LOG_ERROR("{}", status.ToString());
        return std::unexpected{make_error(Error::kCreateRocksDBEngineFailed)};
    }

    return RocksDBEngine{db, checkpoint, write_options, read_options};
}

void vosfs::raft::detail::RocksDBEngine::set_write_options(
    const rocksdb::WriteOptions& write_options) {
    write_options_ = write_options;
}

void vosfs::raft::detail::RocksDBEngine::set_read_options(
    const rocksdb::ReadOptions& read_options) {
    read_options_ = read_options;
}

auto vosfs::raft::detail::RocksDBEngine::clear() const -> rocksdb::Status {
    rocksdb::WriteBatch batch;
    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(read_options_));

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        batch.Delete(it->key());
    }

    if (auto status = it->status(); !status.ok()) {
        return status;
    }

    return db_->Write(write_options_, &batch);
}

auto vosfs::raft::detail::RocksDBEngine::create_checkpoint(const std::filesystem::path& snap_path) const -> rocksdb::Status {
    return checkpoint_->CreateCheckpoint(snap_path);
}

auto vosfs::raft::detail::RocksDBEngine::put(
    const rocksdb::Slice& key,
    const rocksdb::Slice& value) const -> rocksdb::Status {
    return db_->Put(write_options_, key, value);
}

auto vosfs::raft::detail::RocksDBEngine::get(
    const rocksdb::Slice& key,
    std::string* value) const -> rocksdb::Status {
    return db_->Get(read_options_, key, value);
}

auto vosfs::raft::detail::RocksDBEngine::truncate(
    const rocksdb::Slice& key) const -> rocksdb::Status {
    return db_->Delete(write_options_, key);
}

auto vosfs::raft::detail::RocksDBEngine::batch_write(
    rocksdb::WriteBatch& write_batch) const -> rocksdb::Status {
    return db_->Write(write_options_, &write_batch);
}

auto vosfs::raft::detail::RocksDBEngine::multi_get(const std::vector<rocksdb::Slice>& keys,
    std::vector<std::string>& values) const -> std::vector<rocksdb::Status> {
    return db_->MultiGet(read_options_, keys, &values);
}
