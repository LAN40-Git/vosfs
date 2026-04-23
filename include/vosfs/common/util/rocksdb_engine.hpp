#pragma once
#include <rocksdb/db.h>
#include <rocksdb/utilities/checkpoint.h>
#include <filesystem>
#include <kosio/common/debug.hpp>
#include "vosfs/common/error.hpp"

namespace vosfs::util{
class RocksDBEngine {
private:
    explicit RocksDBEngine(
        rocksdb::DB* db,
        rocksdb::Checkpoint* checkpoint,
        const rocksdb::WriteOptions& write_options,
        const rocksdb::ReadOptions&  read_options)
        : db_(db)
        , checkpoint_(checkpoint)
        , write_options_(write_options)
        , read_options_(read_options) {}

public:
    ~RocksDBEngine() {
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

    RocksDBEngine(const RocksDBEngine&) = delete;
    auto operator=(const RocksDBEngine&) = delete;

    RocksDBEngine(RocksDBEngine&& other) noexcept
        : db_(other.db_)
        , checkpoint_(other.checkpoint_)
        , write_options_(other.write_options_)
        , read_options_(other.read_options_) {
        other.db_ = nullptr;
        other.checkpoint_ = nullptr;
    }

    auto operator=(RocksDBEngine&& other) noexcept -> RocksDBEngine& {
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

public:
    static auto create(
        const rocksdb::Options &db_options,
        const rocksdb::WriteOptions& write_options,
        const rocksdb::ReadOptions& read_options,
        const std::filesystem::path& db_path) -> Result<RocksDBEngine> {
        std::error_code ec;
        std::filesystem::create_directories(db_path, ec);
        if (ec) {
            LOG_ERROR("{}", ec.message());
            return std::unexpected{make_error(Error::kDatabaseConnectionFailed)};
        }

        rocksdb::DB* db = nullptr;
        rocksdb::Checkpoint* checkpoint = nullptr;
        auto status = rocksdb::DB::Open(db_options, db_path, &db);

        if (!status.ok()) {
            LOG_ERROR("{}", status.ToString());
            return std::unexpected{make_error(Error::kDatabaseConnectionFailed)};
        }

        status = rocksdb::Checkpoint::Create(db, &checkpoint);

        if (!status.ok()) {
            LOG_ERROR("{}", status.ToString());
            return std::unexpected{make_error(Error::kDatabaseConnectionFailed)};
        }

        return RocksDBEngine{db, checkpoint, write_options, read_options};
    }

public:
    void set_write_options(const rocksdb::WriteOptions &write_options) {
        write_options_ = write_options;
    }

    void set_read_options(const rocksdb::ReadOptions &read_options) {
        read_options_ = read_options;
    }

public:
    [[nodiscard]]
    auto create_checkpoint(const std::filesystem::path& snap_path) const -> rocksdb::Status {
        return checkpoint_->CreateCheckpoint(snap_path);
    }

    [[nodiscard]]
    auto put(const rocksdb::Slice& key, const rocksdb::Slice& value) const -> rocksdb::Status {
        return db_->Put(write_options_, key, value);
    }

    [[nodiscard]]
    auto get(const rocksdb::Slice& key, std::string* value) const -> rocksdb::Status {
        return db_->Get(read_options_, key, value);
    }

    [[nodiscard]]
    auto truncate(const rocksdb::Slice& key) const -> rocksdb::Status {
        return db_->Delete(write_options_, key);
    }

    [[nodiscard]]
    auto batch_write(rocksdb::WriteBatch& write_batch) const -> rocksdb::Status {
        rocksdb::Status code;
        return db_->Write(write_options_, &write_batch);
    }

    [[nodiscard]]
    auto multi_get(const std::vector<rocksdb::Slice>& keys, std::vector<std::string> &values)
        const -> std::vector<rocksdb::Status> {
        return db_->MultiGet(read_options_, keys, &values);
    }

    auto clear() const -> rocksdb::Status {
        rocksdb::WriteBatch batch;
        std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(read_options_));

        for (it->SeekToFirst(); it->Valid(); it->Next()) {
            batch.Delete(it->key());
        }

        if (auto status = it->status(); !status.ok()) {
            return status;
        }

        if (batch.Count() == 0) {
            return rocksdb::Status::OK();
        }

        return db_->Write(write_options_, &batch);
    }

private:
    rocksdb::DB          *db_;
    rocksdb::Checkpoint  *checkpoint_;
    rocksdb::WriteOptions write_options_;
    rocksdb::ReadOptions  read_options_;
};
} // namespace vosfs::util