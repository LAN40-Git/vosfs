#pragma once
#include <deque>
#include "raft.pb.h"
#include "auth.pb.h"
#include "vosfs/rpc/types.hpp"

namespace vosfs::util{
class MessageFactory {
public:
    MessageFactory() = delete;

public:
    [[nodiscard]]
    static auto make_request_vote_request(
        uint64_t term,
        uint64_t candidate_id,
        uint64_t last_log_index,
        uint64_t last_log_term) -> raft::RequestVoteRequest;

    [[nodiscard]]
    static auto make_request_vote_response(
        std::span<char> resp_payload,
        uint64_t id,
        uint64_t term,
        bool vote_granted) -> rpc::RpcResult;

    [[nodiscard]]
    static auto make_append_entries_request(
        uint64_t term,
        uint64_t leader_id,
        uint64_t prev_log_index,
        uint64_t prev_log_term,
        std::vector<raft::LogEntry>& entries,
        uint64_t leader_commit) -> raft::AppendEntriesRequest;

    [[nodiscard]]
    static auto make_append_entries_response(
        std::span<char> resp_payload,
        uint64_t id,
        uint64_t term,
        bool success,
        uint64_t last_log_index,
        std::optional<uint64_t> conflict_index = std::nullopt) -> rpc::RpcResult;

    [[nodiscard]]
    static auto make_install_snapshot_request(
        uint64_t term,
        uint64_t leader_id,
        uint64_t last_included_index,
        uint64_t last_included_term,
        uint64_t offset,
        std::string&& data,
        bool done = false) -> raft::InstallSnapshotRequest;

    [[nodiscard]]
    static auto make_install_snapshot_response(
        std::span<char> resp_payload,
        uint64_t term) -> rpc::RpcResult;

    [[nodiscard]]
    static auto make_transmit_file_request(
        uint64_t parent_ino,
        google::protobuf::RepeatedPtrField<raft::ChunkMetadata>* chunk_metadata) -> raft::TransmitFileRequest;

    [[nodiscard]]
    static auto make_transmit_file_response(
        std::span<char> resp_payload,
        bool success,
        std::string&& path,
        google::protobuf::RepeatedPtrField<raft::ChunkMetadata>* chunk_metadata) -> rpc::RpcResult;

    [[nodiscard]]
    static auto make_put_user_request(
        std::string&& name,
        std::string&& hashed_password,
        int role) -> auth::PutUserRequest;

    [[nodiscard]]
    static auto make_put_user_response(
        std::span<char> resp_payload,
        bool success,
        std::string&& msg) -> rpc::RpcResult;

    [[nodiscard]]
    static auto make_get_user_request(
        std::string&& name,
        std::string&& hashed_password,
        int role) -> auth::GetUserRequest;

    [[nodiscard]]
    static auto make_get_user_response(
        std::span<char> resp_payload,
        bool success,
        std::string&& msg,
        uint64_t uid = 0,
        std::string&& name = "",
        int role = auth::kUser,
        int64_t create_time = 0) -> rpc::RpcResult;

    [[nodiscard]]
    static auto make_update_user_name_request(
        uint64_t uid,
        std::string&& name) -> auth::UpdateUserNameRequest;

    [[nodiscard]]
    static auto make_update_user_name_response(
        std::span<char> resp_payload,
        bool success,
        std::string&& msg) -> rpc::RpcResult;

    [[nodiscard]]
    static auto make_update_user_password_request(
        uint64_t uid,
        std::string&& hashed_password) -> auth::UpdateUserPasswordRequest;

    [[nodiscard]]
    static auto make_update_user_password_response(
        std::span<char> resp_payload,
        bool success,
        std::string&& msg) -> rpc::RpcResult;

    [[nodiscard]]
    static auto make_update_user_role_request(
        uint64_t uid,
        int role) -> auth::UpdateUserRoleRequest;

    [[nodiscard]]
    static auto make_update_user_role_response(
        std::span<char> resp_payload,
        bool success,
        std::string&& msg) -> rpc::RpcResult;

    [[nodiscard]]
    static auto make_delete_user_request(
        uint64_t uid,
        std::string&& hashed_password) -> auth::DeleteUserRequest;

    [[nodiscard]]
    static auto make_delete_user_response(
        std::span<char> resp_payload,
        bool success,
        std::string&& msg) -> rpc::RpcResult;
};
} // namespace vosfs::util