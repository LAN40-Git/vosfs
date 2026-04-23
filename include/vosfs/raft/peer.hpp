#pragma once
#include <vrpc/net/tcp/tcp_client.hpp>
#include "raftpb/raft.pb.h"

namespace vosfs::raft::detail {
using kosio::async::Task;
class Peer {
public:
    explicit Peer(uint64_t id, std::string_view name, std::string_view host, uint16_t port)
        : id_(id)
        , name_(name)
        , rpc_client_(host, port) {}

public:
    Peer(const Peer&) = delete;
    auto operator=(const Peer&) -> Peer& = delete;
    Peer(Peer&& other) noexcept = delete;
    auto operator=(Peer&&) noexcept -> Peer& = delete;

public:
    [[nodiscard]]
    auto id() const noexcept -> uint64_t { return id_; }
    [[nodiscard]]
    auto name() const noexcept -> std::string_view { return name_; }
    [[nodiscard]]
    auto host() const noexcept -> std::string_view { return rpc_client_.host(); }
    [[nodiscard]]
    auto port() const noexcept -> uint16_t { return rpc_client_.port(); }

public:
    [[REMEMBER_CO_AWAIT]]
    auto shutdown() -> Task<void> {
        co_await rpc_client_.shutdown();
    }

public:
    [[REMEMBER_CO_AWAIT]]
    auto send_request_vote_request(
        const RequestVoteRequest& request,
        const std::function<Task<void>(const vrpc::Status& status, const RequestVoteResponse& response)>& callback) -> Task<void> {
        co_await rpc_client_.call_method<RequestVoteRequest, RequestVoteResponse>(
            "raft", "requestvote", request, callback);
    }

    [[REMEMBER_CO_AWAIT]]
    auto send_append_entries_request(
        const AppendEntriesRequest& request,
        const std::function<Task<void>(const vrpc::Status& status, const AppendEntriesResponse& response)>& callback) -> Task<void> {
        co_await rpc_client_.call_method<AppendEntriesRequest, AppendEntriesResponse>(
            "raft", "appendentries", request, callback);
    }

    [[REMEMBER_CO_AWAIT]]
    auto send_install_snapshot_request(
        const InstallSnapshotRequest& request,
        const std::function<Task<void>(const vrpc::Status& status, const InstallSnapshotResponse& response)>& callback) -> Task<void> {
        co_await rpc_client_.call_method<InstallSnapshotRequest, InstallSnapshotResponse>(
            "raft", "installsnapshot", request, callback);
    }

private:
    uint64_t        id_;
    std::string     name_;
    vrpc::TcpClient rpc_client_;
};
} // namespace vosfs::raft::detail