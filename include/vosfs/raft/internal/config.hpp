#pragma once
#include <cstdint>
#include <string_view>

namespace vosfs::raft::detail {
// ====== Port configuration ======
constexpr uint16_t RAFT_PROVIDER_PORT = 8888;

constexpr uint16_t CLIENT_PROVIDER_PORT = 8889;

// ====== Raft internal configuration ======
constexpr std::size_t HEARTBEAT_INTERVAL = 100;

constexpr std::size_t MAX_ENTRIES_PER_APPEND = 100;

constexpr std::size_t SNAPSHOT_INTERVAL = 10000;

// ====== Path configuration ======
constexpr std::string_view DATA_DIR = "/var/lib/vosfs";

constexpr std::string_view DB_DIR = "raft/db";

constexpr std::string_view SNAP_DIR = "raft/snapshots";

// ====== Key configuration ======
constexpr std::string_view HARD_STATE_KEY = "raft/hard_state";

constexpr std::string_view SNAPSHOT_METADATA_KEY = "raft/snapshots/metadata";
} // namespace vosfs::raft::detail