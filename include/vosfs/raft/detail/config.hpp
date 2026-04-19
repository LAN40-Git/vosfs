#pragma once
#include <cstdint>
#include <string_view>

namespace vosfs::raft::detail {
// ====== Port configuration ======
constexpr uint16_t RAFT_RPC_PORT = 8888;

constexpr uint16_t FS_RPC_PORT = 9999;

// ====== Raft internal configuration ======
constexpr std::size_t HEARTBEAT_INTERVAL = 100;

constexpr std::size_t MAX_ENTRIES_PER_APPEND = 100;

constexpr std::size_t SNAPSHOT_INTERVAL = 10;

constexpr std::size_t MAX_SNAPSHOT_CHUNK_SIZE = 1 * 1024 * 1024;

// ====== Path configuration ======
constexpr std::string_view DATA_DIR = "/var/lib/vosfs";

constexpr std::string_view DB_DIR = "raft/db";

constexpr std::string_view SNAP_DIR = "raft/snapshots";
} // namespace vosfs::raft::detail