#include "vosfs/raft/node.hpp"
#include "vosfs/raft/internal/message_factory.hpp"
#include <ranges>

auto vosfs::raft::RaftNode::create(std::string_view data_dir) -> kosio::async::Task<Result<std::unique_ptr<RaftNode>>> {
    // 创建 RPC 服务层
    auto has_raft_rpc_server = co_await rpc::RpcProvider::create(RAFT_RPC_PORT, rpc::RpcProvider::AuthMode::NONE);
    if (!has_raft_rpc_server) {
        co_return std::unexpected{has_raft_rpc_server.error()};
    }
    auto raft_rpc_server = std::move(has_raft_rpc_server.value());

    auto has_client_rpc_server = co_await rpc::RpcProvider::create(CLIENT_RPC_PORT, rpc::RpcProvider::AuthMode::REQUIRED);
    if (!has_client_rpc_server) {
        co_return std::unexpected{has_client_rpc_server.error()};
    }
    auto client_rpc_server = std::move(has_client_rpc_server.value());

    // 创建持久层
    auto has_persister = Persister::create(data_dir);
    if (!has_persister) {
        co_return std::unexpected{has_persister.error()};
    }
    auto persister = std::move(has_persister.value());

    // 创建日志层
    auto has_logs = detail::RaftLog::create(persister);
    if (!has_logs) {
        co_return std::unexpected{has_logs.error()};
    }
    auto logs = std::move(has_logs.value());

    // 创建通信层
    auto has_transport = co_await detail::Transport::create(persister);
    if (!has_transport) {
        co_return std::unexpected{has_transport.error()};
    }

    // 加载 HardState
    auto has_hard_state = persister.load_hard_state();
    if (!has_hard_state) {
        co_return std::unexpected{has_hard_state.error()};
    }
    auto hard_state = std::move(has_hard_state.value());

    // 创建节点

}

auto vosfs::raft::RaftNode::shutdown() -> kosio::async::Task<void> {
    is_shutdown_.store(true, std::memory_order_release);
}

auto vosfs::raft::RaftNode::election_loop() -> kosio::async::Task<void> {
    kosio::util::FastRand rand;
    while (!is_shutdown_.load(std::memory_order_relaxed)) {
        // [150,300]ms timeout
        auto timeout = rand.rand_range(150, 300);
        co_await kosio::time::sleep(timeout);

        if (kosio::util::current_ms() - last_reset_time_.load(std::memory_order_relaxed) < timeout ||
            role_.load(std::memory_order_acquire) == kLeader) {
            continue;
        }

        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);

        // Check again
        if (role_.load(std::memory_order_relaxed) == kLeader) {
            continue;
        }

        co_await do_election();
    }
}

auto vosfs::raft::RaftNode::heartbeat_loop() -> kosio::async::Task<void> {
    while (!is_shutdown_.load(std::memory_order_relaxed)) {
        co_await kosio::time::sleep(HEARTBEAT_INTERVAL);

        if (role_.load(std::memory_order_acquire) != kLeader) {
            continue;
        }

        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);

        if (role_.load(std::memory_order_relaxed) != kLeader) {
            continue;
        }

        do_heartbeat();
    }
}

auto vosfs::raft::RaftNode::do_election() -> kosio::async::Task<void> {
    votes_ = 0;
    auto current_term = hard_state_.current_term();
    hard_state_.set_current_term(current_term+1);
    hard_state_.set_voted_for(transport_.member_id());
    role_.store(kCandidate, std::memory_order_relaxed);
    co_await persist_hard_state();

    // 广播选举请求
    auto request = detail::MessageFactory::make_request_vote_request(
        hard_state_.current_term(),
        transport_.member_id(),
        logs_.last_log_index(),
        logs_.last_log_term());

    kosio::spawn(transport_.broadcast_request(
        rpc::ServiceType::kRaft,
        rpc::MethodType::kRaftRequestVote,
        request.SerializeAsString(),
        [this](std::string_view resp_payload) -> kosio::async::Task<void> {
            co_await this->handle_request_vote_response(resp_payload);
        }));

    // 为自己投票并尝试成为 Leader（当只有一个节点时生效）
    ++votes_;
    if (votes_ > transport_.peer_count() / 2 &&
            role_.load(std::memory_order_relaxed) == kCandidate) {
        co_await become_leader();
    }
}

void vosfs::raft::RaftNode::do_heartbeat() {
    auto current_term = hard_state_.current_term();
    auto leader_id  = transport_.member_id();
    auto commit_index = commit_index_;
    auto last_log_index = logs_.last_log_index();

    // 发送心跳
    for (auto& [member_id, next_index] : next_index_) {
        if (member_id == leader_id) {
            continue;
        }

        std::vector<LogEntry> entries;
        if (next_index <= last_log_index) {
            if (next_index <= logs_.last_included_index()) {
                // 若该节点此时未接收快照，则开始发送快照
                if (snapshot_context_[member_id] == 0) {
                    send_snapshot(member_id, 0);
                }
                continue;
            }

            auto batch_size = std::min(MAX_ENTRIES_PER_APPEND, last_log_index - next_index + 1);
            entries = logs_.get_entries(next_index, batch_size);
        }

        auto prev_log_index = next_index - 1;
        auto prev_log_term = logs_.get_term(prev_log_index);

        auto request = detail::MessageFactory::make_append_entries_request(
            current_term,
            leader_id,
            prev_log_index,
            prev_log_term,
            entries,
            commit_index);

        kosio::spawn(transport_.unicast_request(
            member_id,
            rpc::ServiceType::kRaft,
            rpc::MethodType::kRaftAppendEntries,
            request.SerializeAsString(),
            [this](std::string_view resp_payload) -> kosio::async::Task<void> {
                co_await this->handle_append_entries_response(resp_payload);
            }));
    }
}

auto vosfs::raft::RaftNode::increase_term_to(uint64_t term) -> kosio::async::Task<void> {
    votes_ = 0;
    hard_state_.clear_voted_for();
    hard_state_.set_current_term(term);
    role_.store(kFollower, std::memory_order_release);
    co_await persist_hard_state();
}

auto vosfs::raft::RaftNode::become_leader() -> kosio::async::Task<void> {
    votes_ = 0;
    hard_state_.clear_voted_for();
    role_.store(kLeader, std::memory_order_release);
    co_await persist_hard_state();
    leader_id_ = transport_.member_id();
    // 更新每个Raft节点的 next_index 和 match_index
    auto& peers = transport_.peers();
    for (const auto& peer : peers | std::views::values) {
        next_index_[peer.member_id()] = logs_.last_log_index() + 1;
        match_index_[peer.member_id()] = 0;
    }
}

void vosfs::raft::RaftNode::apply_to_state_machine() {
    auto commit_index = commit_index_;
    while (last_applied_ < commit_index) {
        ++last_applied_;
        state_machine_.apply(logs_.get_entry(last_applied_));
        // TODO: 尝试创建快照

    }
}

auto vosfs::raft::RaftNode::persist_hard_state() -> kosio::async::Task<void> {
    auto ret = persister_.save_hard_state(hard_state_);
    if (!ret) {
        LOG_FATAL("{}", ret.error());
        co_await shutdown();
    }
}

void vosfs::raft::RaftNode::send_snapshot(uint64_t member_id, uint64_t offset) {
    auto current_term = hard_state_.current_term();
    auto leader_id = transport_.member_id();
    auto last_included_index = logs_.last_included_index();
    auto last_included_term = logs_.last_included_term();
    auto size = std::min(MAX_SNAPSHOT_CHUNK_SIZE, snapshot_data_.size() - offset);
    auto data = snapshot_data_.substr(offset, size);
    snapshot_context_[member_id] += size;
    auto done = snapshot_context_[member_id] >= snapshot_data_.size();

    auto request = detail::MessageFactory::make_install_snapshot_request(
        current_term,
        leader_id,
        last_included_index,
        last_included_term,
        offset,
        std::move(data),
        done);

    kosio::spawn(transport_.unicast_request(
        member_id,
        rpc::ServiceType::kRaft,
        rpc::MethodType::kRaftInstallSnapshot,
        request.SerializeAsString(),
        [this](std::string_view resp_payload) -> kosio::async::Task<void> {
            co_await this->handle_install_snapshot_response(resp_payload);
        }));
}

auto vosfs::raft::RaftNode::handle_request_vote_request(
    std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
    RequestVoteRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
        co_return rpc::make_result(rpc::RpcResult::kMessageParseFailed);
    }

    auto term = request.term();
    auto candidate_id = request.candidate_id();
    auto last_log_index = request.last_log_index();
    auto last_log_term = request.last_log_term();

    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    auto current_term = hard_state_.current_term();
    if (term < current_term) {
        co_return detail::MessageFactory::make_request_vote_response(resp_payload, transport_.member_id(), current_term, false);
    }

    if (term > current_term) {
        co_await increase_term_to(term);
        current_term = term;
    }

    bool up_to_date_log{false};

    // 判断日志是否最新
    if (last_log_index > logs_.last_log_index() ||
        (last_log_index == logs_.last_log_index() &&
            last_log_term == logs_.last_log_term())) {
        up_to_date_log = true;
    }

    bool can_vote = (!hard_state_.has_voted_for() || hard_state_.voted_for() == candidate_id) && up_to_date_log;

    if (can_vote) {
        hard_state_.set_voted_for(candidate_id);
    }

    co_return detail::MessageFactory::make_request_vote_response(resp_payload, transport_.member_id(), current_term, can_vote);
}

auto vosfs::raft::RaftNode::handle_append_entries_request(
    std::string_view req_payload,
    std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
    AppendEntriesRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) [[unlikely]] {
        co_return rpc::make_result(rpc::RpcResult::kMessageParseFailed);
    }

    auto leader_id = request.leader_id();
    auto term = request.term();
    auto prev_log_index = request.prev_log_index();
    auto prev_log_term = request.prev_log_term();
    auto entries = request.entries();
    auto leader_commit = request.leader_commit();

    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    auto current_term = hard_state_.current_term();
    if (term < current_term) {
        co_return detail::MessageFactory::make_append_entries_response(
            resp_payload, transport_.member_id(), current_term, false, logs_.last_log_index());
    }

    if (term > current_term) {
        co_await increase_term_to(term);
        current_term = term;
    }

    leader_id_ = leader_id;
    last_reset_time_.store(kosio::util::current_ms(), std::memory_order_relaxed);

    // 判断追加日志的上一条日志是否存在与本地日志中
    bool log_ok{false};
    if (prev_log_index <= logs_.last_log_index() &&
        prev_log_index >= logs_.last_included_index() &&
        prev_log_term == logs_.get_term(prev_log_index)) {
        log_ok = true;
    }

    if (!log_ok) {
        co_return detail::MessageFactory::make_append_entries_response(
            resp_payload, transport_.member_id(),  current_term, false, logs_.last_log_index(), prev_log_index);
    }

    if (!entries.empty()) {
        // 删除冲突的日志
        for (const auto& entry : entries) {
            auto idx = entry.index();
            if (idx > logs_.last_log_index()) {
                break;
            }

            if (logs_.get_term(idx) != entry.term()) {
                auto ret = persister_.truncate_entries(idx, logs_.last_log_index());
                if (!ret) {
                    LOG_WARN("{}", ret.error());
                    co_return detail::MessageFactory::make_append_entries_response(
                        resp_payload, transport_.member_id(), current_term, false, logs_.last_log_index());
                }
                logs_.truncate_entries_before(idx);
                break;
            }
        }

        // 追加并持久化日志
        auto ret = persister_.save_entries(entries);
        if (!ret) {
            LOG_WARN("{}", ret.error());
            co_return detail::MessageFactory::make_append_entries_response(
                resp_payload, transport_.member_id(), current_term, false, logs_.last_log_index());
        }
        logs_.append_entries(entries);
    }

    if (leader_commit > commit_index_) {
        commit_index_ = std::min(logs_.last_log_index(), leader_commit);
        apply_to_state_machine();
    }

    co_return detail::MessageFactory::make_append_entries_response(
        resp_payload, transport_.member_id(), current_term, true, logs_.last_log_index());
}

auto vosfs::raft::RaftNode::handle_install_snapshot_request(
    std::string_view req_payload,
    std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
    InstallSnapshotRequest request;
    if (!request.ParseFromString(req_payload.data())) {
        co_return rpc::make_result(rpc::RpcResult::kMessageParseFailed);
    }

    auto term = request.term();
    auto leader_id = request.leader_id();
    [[maybe_unused]] auto last_included_index = request.last_included_index();
    [[maybe_unused]] auto last_included_term = request.last_included_term();
    auto offset = request.offset();
    auto& data = request.data();
    auto done = request.done();

    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    auto current_term = hard_state_.current_term();
    if (term < current_term) {
        co_return detail::MessageFactory::make_install_snapshot_response(resp_payload, current_term);
    }

    if (term > current_term) {
        co_await increase_term_to(term);
        current_term = term;
    }

    leader_id_ = leader_id;
    last_reset_time_.store(kosio::util::current_ms(), std::memory_order_relaxed);

    if (offset == 0) {
        snapshot_data_.clear();
    }

    snapshot_data_.append(data);

    if (!done) {
        co_return detail::MessageFactory::make_install_snapshot_response(resp_payload, current_term);
    }

    if (auto ret = persister_.save_snapshot(snapshot_data_); !ret) {
        LOG_WARN("{}", ret.error());
    }

    Snapshot snapshot;
    if (!snapshot.ParseFromString(snapshot_data_)) {
        LOG_WARN("failed to parse from snapshot data.");
        co_return detail::MessageFactory::make_install_snapshot_response(resp_payload, current_term);
    }

    // 状态机应用快照-更新 Inode 信息
    state_machine_.apply_snapshot(snapshot);
    // 日志层应用快照-更新快照元数据
    logs_.apply_snapshot(snapshot);
    // 通信层应用快照-集群配置
    if (auto ret = co_await transport_.apply_snapshot(snapshot); !ret) {
        // 关闭节点
        co_await shutdown();
    }

    co_return detail::MessageFactory::make_install_snapshot_response(resp_payload, current_term);
}

auto vosfs::raft::RaftNode::handle_request_vote_response(std::string_view resp_payload) -> kosio::async::Task<void> {
    RequestVoteResponse response;
    if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
        co_return;
    }

    [[maybe_unused]] auto id = response.id();
    auto term = response.term();
    auto vote_granted = response.vote_granted();

    if (term < hard_state_.current_term()) {
        co_return;
    }

    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    auto current_term = hard_state_.current_term();
    if (term < current_term) {
        co_return;
    }

    if (term > current_term) {
        co_await increase_term_to(term);
        co_return;
    }

    if (role_.load(std::memory_order_relaxed) == kLeader) {
        co_return;
    }

    if (vote_granted) {
        if (++votes_ > transport_.peer_count() / 2 &&
            role_.load(std::memory_order_relaxed) == kCandidate) {
            co_await become_leader();
        }
    }
}

auto vosfs::raft::RaftNode::handle_append_entries_response(std::string_view resp_payload) -> kosio::async::Task<void> {
    AppendEntriesResponse response;
    if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
        LOG_ERROR("Failed to parse request vote response");
        co_return;
    }

    auto id = response.id();
    auto term = response.term();
    auto success = response.success();
    auto last_log_index = response.last_log_index();

    if (term < hard_state_.current_term() ||
        role_.load(std::memory_order_acquire) != kLeader) {
        co_return;
    }

    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    auto current_term = hard_state_.current_term();
    if (term < current_term ||
        role_.load(std::memory_order_relaxed) != kLeader) {
        co_return;
    }

    if (term > current_term) {
        co_await increase_term_to(term);
        co_return;
    }

    if (!success) {
        if (response.has_conflict_index()) {
            next_index_[id] = response.conflict_index();
        }
        co_return;
    }

    match_index_[id] = last_log_index;
    next_index_[id] = last_log_index + 1;
    std::vector<uint64_t> idxs{};
    idxs.reserve(transport_.peer_count());
    for (auto idx: match_index_ | std::views::values) {
        idxs.push_back(idx);
    }
    std::ranges::sort(idxs);
    if (idxs[idxs.size()/2] > commit_index_) {
        commit_index_ = idxs[idxs.size()/2];
        apply_to_state_machine();
    }
}

auto vosfs::raft::RaftNode::handle_install_snapshot_response(std::string_view resp_payload) -> kosio::async::Task<void> {
    InstallSnapshotResponse response;
    if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
        LOG_ERROR("failed to parse request vote response");
        co_return;
    }

    auto id = response.id();
    auto term = response.term();

    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    auto current_term = hard_state_.current_term();
    if (term > current_term) {
        co_await increase_term_to(term);
        co_return;
    }

    auto offset = snapshot_context_[id];

    // 检查快照是否传送完毕
    if (offset >= snapshot_data_.size()) {
        snapshot_context_[id] = 0;
        co_return;
    }

    send_snapshot(id, offset);
}
