#pragma once
#include <vrpc/net/tcp/tcp_server.hpp>
#include "vosfs/common/error.hpp"
#include "raftpb/raft.pb.h"

namespace vosfs::storage {
class DataNode {
public:
    explicit DataNode(std::string_view host, uint16_t port, std::string_view block_dir)
        : host_(host)
        , port_(port)
        , block_dir_(block_dir) {}

public:
    [[REMEMBER_CO_AWAIT]]
    auto wait() -> kosio::async::Task<void>;

private:
    [[REMEMBER_CO_AWAIT]]
    auto handle_upload_block_request(const raft::UploadBlockRequest& request) -> kosio::async::Task<raft::UploadBlockResponse>;

    [[REMEMBER_CO_AWAIT]]
    auto handle_download_block_request(const raft::UploadBlockRequest& request) -> kosio::async::Task<raft::UploadBlockResponse>;

private:
    std::string     host_;
    uint16_t        port_;
    std::string     block_dir_;
};
} // namespace vosfs::storage