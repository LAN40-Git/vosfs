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
constexpr std::string_view CURRENT_TERM_KEY = "raft/current_term";

constexpr std::string_view VOTED_FOR_KEY = "raft/voted_for";

constexpr std::string_view SNAPSHOT_LAST_INCLUDED_INDEX_KEY = "raft/snapshot/last_included_index";

constexpr std::string_view SNAPSHOT_LAST_INCLUDED_TERM_KEY = "raft/snapshot/last_included_term";
} // namespace vosfs::raft::detail