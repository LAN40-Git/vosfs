#pragma once
#include <coroutine>
#include <ranges>
#include <unordered_map>
#include "config.hpp"
#include "raftpb/raft.pb.h"

namespace vosfs::raft::detail {
struct PendingRequest {
    std::coroutine_handle<> handle;
    PendingResponse response;
};

class StateMachine {
    using InodeMap = std::unordered_map<uint64_t, Inode>;
    using DirEntryMap = std::unordered_map<std::string, DirEntry>;
    using PendingRequestMap = std::unordered_map<uint64_t, PendingRequest>;
public:
    explicit StateMachine(const Config& config);

public:
    void load_snapshot(const Snapshot& snapshot);
    void take_snapshot(Snapshot& snapshot);
    void apply_entry(const LogEntry& entry);
    void apply_entry_no_response(const LogEntry& entry);
    [[nodiscard]]
    auto append_request(uint64_t log_index, PendingRequest request) -> PendingRequest&;

public:
    void ls(const ListDirRequest& request, ListDirResponse& response);
    void prepare_upload_file(PrepareUploadFileRequest& request, PrepareUploadFileResponse& response);

private:
    void mkdir(const MakeDirRequest& request);
    void mkdir(const MakeDirRequest& request, MakeDirResponse* response);

private:
    InodeMap              inodes_;
    DirEntryMap           dir_entries_;
    std::vector<NodeInfo> data_nodes_;
    uint64_t              next_ino_;
    PendingRequestMap     pending_requests_;
};
} // namespace vosfs::raft::detail