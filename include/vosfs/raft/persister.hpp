#pragma once
#include "raftpb/raft.pb.h"
#include "vosfs/raft/config.hpp"
#include "vosfs/common/util/rocksdb_engine.hpp"

namespace vosfs::raft::detail {
class Persister {
    static constexpr std::string_view HARD_STATE_KEY = "hard_state";

private:
    explicit Persister(util::RocksDBEngine engine)
        : engine_(std::move(engine)) {}

public:
    Persister(Persister&& other) noexcept = default;
    auto operator=(Persister&& other) noexcept -> Persister& = default;

public:
    static auto create(const std::filesystem::path& db_dir) -> Result<Persister>;

public:
    void save_hard_state(const HardState& hard_state) const;
    [[nodiscard]]
    auto load_hard_state() const -> Result<HardState>;
    void save_entry(const LogEntry& entry) const;
    void save_entries(const google::protobuf::RepeatedPtrField<LogEntry>& entries) const;
    [[nodiscard]]
    auto load_entries(uint64_t last_included_index) const -> Result<std::vector<LogEntry>>;
    auto truncate_entries(uint64_t start_index, uint64_t end_index) const -> void;

private:
    static auto get_entry_key(uint64_t index) -> std::string {
        return "log" + std::to_string(index);
    }

private:
    util::RocksDBEngine engine_;
};
} // namespace vosfs::raft::detail