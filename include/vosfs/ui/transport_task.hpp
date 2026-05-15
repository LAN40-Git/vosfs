#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace vosfs::ui::detail {
struct BlockInfo {
    uint64_t remaing_times;
    uint64_t id;
    uint64_t ino;
    uint64_t offset;
    uint64_t size;
    std::unordered_map<uint64_t, bool> data_node_ids;
};

struct TransportTask {
    uint64_t ino;
    uint64_t total_blocks;
    uint64_t done_blocks;
    std::string local_path;
    std::string remote_path;
    std::unordered_map<uint64_t, BlockInfo> blocks;
};
} // namespace vosfs::ui::detail