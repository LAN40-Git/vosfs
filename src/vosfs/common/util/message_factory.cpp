#include "vosfs/common/util/message_factory.hpp"

auto vosfs::util::MessageFactory::make_request_vote_request(
    uint64_t term,
    uint64_t candidate_id,
    uint64_t last_log_index,
    uint64_t last_log_term) -> raft::RequestVoteRequest {
    raft::RequestVoteRequest request;
    request.set_term(term);
    request.set_candidate_id(candidate_id);
    request.set_last_log_index(last_log_index);
    request.set_last_log_term(last_log_term);
    return request;
}

auto vosfs::util::MessageFactory::make_request_vote_response(
    std::span<char> resp_payload,
    uint64_t id,
    uint64_t term,
    bool vote_granted) -> vrpc::InvokeResult {
    raft::RequestVoteResponse response;
    response.set_id(id);
    response.set_term(term);
    response.set_vote_granted(vote_granted);
    return rpc::serialize_response(response, resp_payload);
}

auto vosfs::util::MessageFactory::make_append_entries_request(
    uint64_t term,
    uint64_t leader_id,
    uint64_t prev_log_index,
    uint64_t prev_log_term,
    std::vector<raft::LogEntry>& entries,
    uint64_t leader_commit) -> raft::AppendEntriesRequest {
    raft::AppendEntriesRequest request;
    request.set_term(term);
    request.set_leader_id(leader_id);
    request.set_prev_log_index(prev_log_index);
    request.set_prev_log_term(prev_log_term);
    for (auto& entry : entries) {
        request.mutable_entries()->Add(std::move(entry));
    }
    request.set_leader_commit(leader_commit);
    return request;
}

auto vosfs::util::MessageFactory::make_append_entries_response(
    std::span<char> resp_payload,
    uint64_t id,
    uint64_t term,
    bool success,
    uint64_t last_log_index,
    std::optional<uint64_t> conflict_index) -> vrpc::InvokeResult {
    raft::AppendEntriesResponse response;
    response.set_id(id);
    response.set_term(term);
    response.set_success(success);
    response.set_last_log_index(last_log_index);
    if (conflict_index.has_value()) {
        response.set_conflict_index(conflict_index.value());
    }
    return rpc::serialize_response(response, resp_payload);
}

auto vosfs::util::MessageFactory::make_install_snapshot_request(
    uint64_t term,
    uint64_t leader_id,
    uint64_t last_included_index,
    uint64_t last_included_term,
    uint64_t offset,
    std::string&& data,
    bool done) -> raft::InstallSnapshotRequest {
    raft::InstallSnapshotRequest request;
    request.set_term(term);
    request.set_leader_id(leader_id);
    request.set_last_included_index(last_included_index);
    request.set_last_included_term(last_included_term);
    request.set_offset(offset);
    request.set_data(std::move(data));
    request.set_done(done);
    return request;
}

auto vosfs::util::MessageFactory::make_install_snapshot_response(
    std::span<char> resp_payload,
    uint64_t term) -> vrpc::InvokeResult {
    raft::InstallSnapshotResponse response;
    response.set_term(term);
    return rpc::serialize_response(response, resp_payload);
}

auto vosfs::util::MessageFactory::make_transmit_file_request(
    uint64_t parent_ino,
    google::protobuf::RepeatedPtrField<raft::ChunkMetadata>* chunk_metadata) -> raft::TransmitFileRequest {
    raft::TransmitFileRequest request;
    request.set_parent_ino(parent_ino);
    if (chunk_metadata) {
        request.mutable_chunk_metadata()->Swap(chunk_metadata);
    }
    return request;
}

auto vosfs::util::MessageFactory::make_transmit_file_response(
    std::span<char> resp_payload,
    bool success,
    std::string&& path,
    google::protobuf::RepeatedPtrField<raft::ChunkMetadata>* chunk_metadata) -> vrpc::InvokeResult {
    raft::TransmitFileResponse response;
    response.set_success(success);
    response.set_path(std::move(path));
    if (chunk_metadata) {
        response.mutable_chunk_metadata()->Swap(chunk_metadata);
    }
    return rpc::serialize_response(response, resp_payload);
}

auto vosfs::util::MessageFactory::make_put_user_request(
    std::string&& name,
    std::string&& hashed_password,
    int role) -> auth::PutUserRequest {
    auth::PutUserRequest request;
    request.set_name(std::move(name));
    request.set_hashed_password(std::move(hashed_password));
    request.set_role(static_cast<auth::Role>(role));
    return request;
}

auto vosfs::util::MessageFactory::make_put_user_response(
    std::span<char> resp_payload,
    bool success,
    std::string&& msg) -> vrpc::InvokeResult {
    auth::PutUserResponse response;
    response.set_success(success);
    response.set_msg(std::move(msg));
    return rpc::serialize_response(response, resp_payload);
}

auto vosfs::util::MessageFactory::make_get_user_request(
    std::string&& name,
    std::string&& hashed_password,
    int role) -> auth::GetUserRequest {
    auth::GetUserRequest request;
    request.set_name(std::move(name));
    request.set_hashed_password(std::move(hashed_password));
    request.set_role(static_cast<auth::Role>(role));
    return request;
}

auto vosfs::util::MessageFactory::make_get_user_response(
    std::span<char> resp_payload,
    bool success,
    std::string&& msg,
    uint64_t uid,
    std::string&& name,
    int role,
    int64_t create_time) -> vrpc::InvokeResult {
    auth::GetUserResponse response;
    response.set_success(success);
    response.set_msg(std::move(msg));
    response.set_uid(uid);
    response.set_name(std::move(name));
    response.set_role(static_cast<auth::Role>(role));
    response.set_create_time(create_time);
    return rpc::serialize_response(response, resp_payload);
}

auto vosfs::util::MessageFactory::make_update_user_name_request(
    uint64_t uid,
    std::string&& name) -> auth::UpdateUserNameRequest {
    auth::UpdateUserNameRequest request;
    request.set_uid(uid);
    request.set_name(std::move(name));
    return request;
}

auto vosfs::util::MessageFactory::make_update_user_name_response(
    std::span<char> resp_payload,
    bool success,
    std::string&& msg) -> vrpc::InvokeResult {
    auth::UpdateUserNameResponse response;
    response.set_success(success);
    response.set_msg(std::move(msg));
    return rpc::serialize_response(response, resp_payload);
}

auto vosfs::util::MessageFactory::make_update_user_password_request(
    uint64_t uid,
    std::string&& hashed_password) -> auth::UpdateUserPasswordRequest {
    auth::UpdateUserPasswordRequest request;
    request.set_uid(uid);
    request.set_hashed_password(std::move(hashed_password));
    return request;
}

auto vosfs::util::MessageFactory::make_update_user_password_response(
    std::span<char> resp_payload,
    bool success,
    std::string&& msg) -> vrpc::InvokeResult {
    auth::UpdateUserPasswordResponse response;
    response.set_success(success);
    response.set_msg(std::move(msg));
    return rpc::serialize_response(response, resp_payload);
}

auto vosfs::util::MessageFactory::make_update_user_role_request(
    uint64_t uid,
    int role) -> auth::UpdateUserRoleRequest {
    auth::UpdateUserRoleRequest request;
    request.set_uid(uid);
    request.set_role(static_cast<auth::Role>(role));
    return request;
}

auto vosfs::util::MessageFactory::make_update_user_role_response(
    std::span<char> resp_payload,
    bool success,
    std::string&& msg) -> vrpc::InvokeResult {
    auth::UpdateUserRoleResponse response;
    response.set_success(success);
    response.set_msg(std::move(msg));
    return rpc::serialize_response(response, resp_payload);
}

auto vosfs::util::MessageFactory::make_delete_user_request(
    uint64_t uid,
    std::string&& hashed_password) -> auth::DeleteUserRequest {
    auth::DeleteUserRequest request;
    request.set_uid(uid);
    request.set_hashed_password(std::move(hashed_password));
    return request;
}

auto vosfs::util::MessageFactory::make_delete_user_response(
    std::span<char> resp_payload,
    bool success,
    std::string&& msg) -> vrpc::InvokeResult {
    auth::DeleteUserResponse response;
    response.set_success(success);
    response.set_msg(std::move(msg));
    return rpc::serialize_response(response, resp_payload);
}
