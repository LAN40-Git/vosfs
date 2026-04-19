#pragma once
#include "vosfs/common/error.hpp"
#include "vosfs/common/util/noncopyable.hpp"
#include <rocksdb/db.h>
#include <rocksdb/utilities/checkpoint.h>
#include <filesystem>
#include <utility>

namespace vosfs::raft::detail {
class RocksDBEngine : util::Noncopyable {
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
    ~RocksDBEngine();
    RocksDBEngine(RocksDBEngine&& other) noexcept;
    auto operator=(RocksDBEngine&& other) noexcept -> RocksDBEngine&;

public:
    static auto create(
        const rocksdb::Options &db_options,
        const rocksdb::WriteOptions& write_options,
        const rocksdb::ReadOptions& read_options,
        const std::filesystem::path& db_path) -> Result<RocksDBEngine>;

public:
    void set_write_options(const rocksdb::WriteOptions &write_options);

    void set_read_options(const rocksdb::ReadOptions &read_options);

public:
    auto clear() const -> rocksdb::Status;

    [[nodiscard]]
    auto create_checkpoint(const std::filesystem::path& snap_path) const -> rocksdb::Status;

    [[nodiscard]]
    auto put(const rocksdb::Slice& key, const rocksdb::Slice& value) const -> rocksdb::Status;

    [[nodiscard]]
    auto get(const rocksdb::Slice& key, std::string* value) const -> rocksdb::Status;

    [[nodiscard]]
    auto truncate(const rocksdb::Slice& key) const -> rocksdb::Status;

    [[nodiscard]]
    auto batch_write(rocksdb::WriteBatch& write_batch) const -> rocksdb::Status;

    [[nodiscard]]
    auto multi_get(const std::vector<rocksdb::Slice>& keys, std::vector<std::string> &values)
    const -> std::vector<rocksdb::Status>;

private:
    rocksdb::DB          *db_;
    rocksdb::Checkpoint  *checkpoint_;
    rocksdb::WriteOptions write_options_;
    rocksdb::ReadOptions  read_options_;
};
} // namespace vosfs::raft::detail