#pragma once
#include <string>
namespace vosfs::raft::detail {
static constexpr std::string_view RAFT_DB_PATH = "/raft/db";

struct Config {
    // vosfs 数据目录
    std::string data_dir{"var/lib/vosfs"};

    // vosfs 初始化配置文件路径
    std::string config_path{"var/lib/vosfs/conf/raft_config.json"};

    // 心跳间隔（ms）
    std::size_t heartbeat_interval = 50;

    // 单次心跳最大追加日志数量
    std::size_t max_append_entries_size = 64;

    // 生成快照的日志间隔
    std::size_t snapshot_interval = 10;

    // 快照分片大小（bytes）
    std::size_t max_snapshot_chunk_size = 1 * 1024 * 1024;
};
} // namespace vosfs::raft::detail