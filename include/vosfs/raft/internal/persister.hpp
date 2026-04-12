#pragma once
#include "vosfs/raft/internal/config.hpp"
#include "vosfs/raft/internal/rocksdb_engine.hpp"

namespace vosfs::raft::detail {
struct KV {
    std::string key;
    std::string value;
};

class Persister {

private:
    explicit Persister(RocksDBEngine engine)
        : engine_(std::move(engine)) {}

public:
    Persister(Persister&& other) noexcept = default;
    auto operator=(Persister&& other) noexcept -> Persister& = default;

public:
    static auto create(std::string_view data_dir) -> Result<Persister>;

public:
    [[nodiscard]]
    auto persist(std::string_view key, std::string_view value) const -> Result<void>;

    [[nodiscard]]
    auto persist_batch(const std::vector<KV>& kvs) const -> Result<void>;

    [[nodiscard]]
    auto recover(std::string_view key, std::string* value) const -> Result<void>;

    [[nodiscard]]
    auto recover_batch(const std::vector<rocksdb::Slice>& keys, std::vector<std::string>& values) const -> Result<void>;

    [[nodiscard]]
    auto truncate(std::string_view key) const -> Result<void>;

    [[nodiscard]]
    auto truncate_batch(const std::vector<std::string>& keys, uint64_t start_index, std::size_t size) const -> Result<void>;

private:
    RocksDBEngine engine_;
};
} // namespace vosfs::raft::detail