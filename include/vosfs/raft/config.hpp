#pragma once
#include <string>
namespace vosfs::raft::detail {
static constexpr std::size_t MAX_SNAPSHOT_CHUNK_SIZE = 4 * 1024;
struct Config {
    // vosfs 数据目录
    std::string data_dir{"/var/lib/vosfs"};

    // 集群配置文件
    std::string config_path{"/var/lib/vosfs/conf/raft_config.json"};

    // 心跳间隔（ms）
    std::size_t heartbeat_interval = 50;

    // 单次心跳最大追加日志数量
    std::size_t max_append_entries_size = 64;

    // 生成快照的日志间隔
    std::size_t snapshot_interval = 10;
};
} // namespace vosfs::raft::detail