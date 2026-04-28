#include "vosfs/raft/node.hpp"
#include "vosfs/common/util/file.hpp"
#include "vosfs/common/util/time.hpp"

vosfs::raft::RaftNode::RaftNode(
    detail::Snapshotter::Ptr snapshotter,
    detail::StateMachine state_machine,
    detail::Persister persister,
    detail::RaftLog logs,
    detail::Transport transport,
    HardState hard_state)
    : rpc_server_("0.0.0.0", transport.port())
    , snapshotter_(std::move(snapshotter))
    , state_machine_(std::move(state_machine))
    , persister_(std::move(persister))
    , logs_(std::move(logs))
    , transport_(std::move(transport))
    , hard_state_(std::move(hard_state)) {

}

auto vosfs::raft::RaftNode::create(const std::string& config_path) -> Result<std::unique_ptr<RaftNode>> {
    auto has_config = Config::from_json(config_path);
    if (!has_config) {
        return std::unexpected{has_config.error()};
    }
    auto config = std::move(has_config.value());

    auto data_dir = std::filesystem::path(config.data_dir);
    auto snapshot_dir = data_dir / SNAPSHOT_DIR;
    auto db_dir = data_dir / DB_DIR;

    uint64_t last_included_index = 0;
    uint64_t last_included_term = 0;
    std::unordered_map<uint64_t, Inode> inodes{};

    // 初始化快照层
    auto snapshotter = std::make_unique<detail::Snapshotter>(
        (snapshot_dir / SNAPSHOT_FILE_NAME).c_str(),
        (snapshot_dir / SNAPSHOT_TEMP_FILE_NAME).c_str());

    // 尝试加载快照
    if (auto ret = util::get_file_size((snapshot_dir / SNAPSHOT_FILE_NAME).c_str()); ret == -1) {
        return std::unexpected{make_error(Error::kFileError)};
    } else if (ret > 0) {
        auto has_snapshot = snapshotter->load_snapshot();
        if (!has_snapshot) {
            return std::unexpected{has_snapshot.error()};
        }

        auto snapshot = std::move(has_snapshot.value());
        last_included_term = snapshot.last_included_term();
        last_included_index = snapshot.last_included_index();
        auto& inode_map = snapshot.inodes();

        for (const auto& [ino, inode] : inode_map) {
            inodes.emplace(ino, inode);
        }
    }

    // 初始化状态机
    auto state_machine = detail::StateMachine{std::move(inodes)};

    // 初始化持久层
    auto has_persister = detail::Persister::create(db_dir);
    if (!has_persister) {
        return std::unexpected{has_persister.error()};
    }
    auto persister = std::move(has_persister.value());

    // 加载日志
    auto has_entries = persister.load_entries(last_included_index);
    if (!has_entries) {
        return std::unexpected{has_entries.error()};
    }
    auto entries = std::move(has_entries.value());

    // 创建日志层
    auto logs = detail::RaftLog{last_included_index, last_included_term, std::move(entries)};

    // 创建通信层
    auto transport = detail::Transport{config};

    // 初始化 HardState
    auto has_hard_state = persister.load_hard_state();
    if (!has_hard_state) {
        return std::unexpected{has_hard_state.error()};
    }
    auto hard_state = std::move(has_hard_state.value());

    // 初始化节点
    return std::unique_ptr<RaftNode>(new RaftNode{
        std::move(snapshotter),
        std::move(state_machine),
        std::move(persister),
        std::move(logs),
        std::move(transport),
        std::move(hard_state)});
}

auto vosfs::raft::RaftNode::wait() -> Task<void> {
    kosio::spawn(election_loop());
    kosio::spawn(heartbeat_loop());
    co_await rpc_server_
    .register_method<RequestVoteRequest, RequestVoteResponse>(
        "raft", "requestvote",
        [this](const RequestVoteRequest& request) -> Task<RequestVoteResponse> {
            co_return co_await this->handle_request_vote_request(request);
    })
    .register_method<AppendEntriesRequest, AppendEntriesResponse>(
        "raft", "appendentries",
        [this](const AppendEntriesRequest& request) -> Task<AppendEntriesResponse> {
        co_return co_await this->handle_append_entries_request(request);
    })
    .register_method<InstallSnapshotRequest, InstallSnapshotResponse>(
        "raft", "installsnapshot",
        [this](const InstallSnapshotRequest& request) -> Task<InstallSnapshotResponse> {
        co_return co_await this->handle_install_snapshot_request(request);
    })
    .wait();
}

auto vosfs::raft::RaftNode::shutdown() -> Task<void> {
    if (is_shutdown_.load(std::memory_order_acquire)) {
        co_return;
    }

    is_shutdown_.store(true, std::memory_order_release);
    co_await latch_.wait();
    co_await rpc_server_.shutdown();
    co_await transport_.shutdown();
}

auto vosfs::raft::RaftNode::election_loop() -> Task<void> {
    kosio::util::FastRand rand;
    while (!is_shutdown_.load(std::memory_order_relaxed)) {
        // [150,300]ms timeout
        auto election_timeout = rand.rand_range(150, 300);
        co_await kosio::time::sleep(election_timeout);

        auto timeout = util::current_ms() - last_reset_time_.load(std::memory_order_relaxed);

        if (timeout < election_timeout || role_.load(std::memory_order_acquire) == kLeader) {
            continue;
        }
        LOG_INFO("{}", timeout);

        co_await mutex_.lock();

        // Check again
        if (role_.load(std::memory_order_relaxed) == kLeader) {
            mutex_.unlock();
            continue;
        }

        votes_ = 0;
        auto current_term = hard_state_.current_term() + 1;
        hard_state_.set_current_term(current_term);
        hard_state_.set_voted_for(transport_.member_id());
        role_.store(kCandidate, std::memory_order_relaxed);
        persister_.save_hard_state(hard_state_);
        auto member_id = transport_.member_id();
        auto last_log_index = logs_.last_log_index();
        auto last_log_term = logs_.last_log_term();

        // 为自己投票并尝试成为 Leader（当只有一个节点时生效）
        ++votes_;
        if (votes_ > transport_.cluster_size() / 2 &&
            role_.load(std::memory_order_relaxed) == kCandidate) {
            become_leader();
        }
        mutex_.unlock();

        // 广播选举请求
        auto request = make_request_vote_request(
            current_term,
            member_id,
            last_log_index,
            last_log_term);

        co_await transport_.broadcast_request_vote_request(
            request, [this](const vrpc::Status& status, const RequestVoteResponse& response) -> Task<void> {
                if (!status.ok()) {
                    LOG_ERROR("{}", status.message());
                    co_return;
                }
                co_await this->handle_request_vote_response(response);
            });
    }
    latch_.count_down();
}

auto vosfs::raft::RaftNode::heartbeat_loop() -> Task<void> {
    while (!is_shutdown_.load(std::memory_order_relaxed)) {
        co_await kosio::time::sleep(detail::HEARTBEAT_INTERVAL);

        if (role_.load(std::memory_order_acquire) != kLeader) {
            continue;
        }

        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);

        if (role_.load(std::memory_order_relaxed) != kLeader) {
            continue;
        }

        std::unordered_map<uint64_t, AppendEntriesRequest> requests;
        // 构造心跳消息
        for (auto& [member_id, next_index] : next_index_) {
            auto current_term = hard_state_.current_term();
            auto leader_id  = transport_.member_id();
            auto commit_index = commit_index_;
            auto last_log_index = logs_.last_log_index();

            if (member_id == leader_id) {
                continue;
            }

            std::vector<LogEntry> entries;
            // 补发日志或者快照
            if (next_index <= last_log_index) {
                if (next_index <= logs_.last_included_index()) {
                    co_await send_snapshot(member_id);
                    continue;
                }

                auto batch_size = std::min(detail::MAX_APPEND_ENTRIES_SIZE, last_log_index - next_index + 1);
                entries = logs_.get_entries(next_index, batch_size);
            }

            auto prev_log_index = next_index - 1;
            auto prev_log_term = logs_.get_term(prev_log_index);

            auto request = make_append_entries_request(
                current_term,
                leader_id,
                prev_log_index,
                prev_log_term,
                entries,
                commit_index);

            requests.emplace(member_id, request);
        }

        // 发送日志同步请求（心跳）
        for (const auto& [member_id, request] : requests) {
            co_await transport_.unicast_append_entries_request(
                member_id, request,
                [this](const vrpc::Status& status, const AppendEntriesResponse& response) -> Task<void> {
                    if (!status.ok()) {
                        LOG_ERROR("{}", status.message());
                        co_return;
                    }
                    co_await this->handle_append_entries_response(response);
                });
        }
    }
    latch_.count_down();
}

auto vosfs::raft::RaftNode::send_snapshot(uint64_t member_id) -> Task<void> {
    if (snapshot_data_.empty() || co_await snapshotter_->offset_at(member_id) != 0) {
        co_return;
    }

    auto current_term = hard_state_.current_term();
    auto leader_id = transport_.member_id();
    auto last_included_index = logs_.last_included_index();
    auto last_included_term = logs_.last_included_term();

    // 发送快照安装请求
    auto request = make_install_snapshot_request(
        current_term,
        leader_id,
        last_included_index,
        last_included_term);

    if (auto ret = co_await snapshotter_->load_snapshot_data(member_id, request); !ret) {
        LOG_ERROR("{}", ret.error());
        co_return;
    }

    co_await transport_.unicast_install_snapshot_request(
        member_id, request,
        [this](const vrpc::Status& status, const InstallSnapshotResponse& response) -> Task<void> {
            if (!status.ok()) {
                LOG_ERROR("{}", status.message());
            }
            co_await this->handle_install_snapshot_response(response);
        });
}

auto vosfs::raft::RaftNode::apply_entry() -> Task<void> {
    auto commit_index = commit_index_;
    while (last_applied_ < commit_index) {
        ++last_applied_;
        state_machine_.apply_entry(logs_.get_entry(last_applied_));
        if (last_applied_ % detail::SNAPSHOT_INTERVAL == 0) {
            Snapshot snapshot;
            // 创建并保存快照
            auto last_included_index = last_applied_;
            auto last_included_term = logs_.get_term(last_included_index);
            auto& inodes = state_machine_.inodes();

            snapshot.set_last_included_index(last_included_index);
            snapshot.set_last_included_term(last_included_term);

            for (auto& inode : inodes | std::views::values) {
                snapshot.mutable_inodes()->emplace(inode.ino(), inode);
            }

            if (auto ret = co_await snapshotter_->save_snapshot(std::move(snapshot)); !ret) {
                LOG_FATAL("{}", ret.error());
            } else {
                logs_.set_last_included_index(last_included_index);
                logs_.set_last_included_term(last_included_term);
            }
        }
    }
}

void vosfs::raft::RaftNode::increase_term_to(uint64_t term) {
    votes_ = 0;
    hard_state_.clear_voted_for();
    hard_state_.set_current_term(term);
    role_.store(kFollower, std::memory_order_release);
    persister_.save_hard_state(hard_state_);
}

void vosfs::raft::RaftNode::become_leader() {
    votes_ = 0;
    hard_state_.clear_voted_for();
    role_.store(kLeader, std::memory_order_release);
    persister_.save_hard_state(hard_state_);
    leader_id_ = transport_.member_id();
    LOG_INFO("become leader, current_term: {}", hard_state_.current_term());
    // 更新每个Raft节点的 next_index 和 match_index
    auto& peers = transport_.peers();
    for (const auto& peer : peers | std::views::values) {
        next_index_[peer->id()] = logs_.last_log_index() + 1;
        match_index_[peer->id()] = 0;
    }
    // 重置快照偏移
    snapshotter_->reset_offsets();
}

auto vosfs::raft::RaftNode::handle_request_vote_request(
    const RequestVoteRequest& request) -> Task<RequestVoteResponse> {
    auto member_id = transport_.member_id();
    auto term = request.term();
    auto candidate_id = request.candidate_id();
    auto last_log_index = request.last_log_index();
    auto last_log_term = request.last_log_term();

    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    auto current_term = hard_state_.current_term();
    if (term < current_term) {
        co_return make_request_vote_response(member_id, current_term, false);
    }

    if (term > current_term) {
        increase_term_to(term);
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

    co_return make_request_vote_response(member_id, current_term, can_vote);
}

auto vosfs::raft::RaftNode::handle_append_entries_request(
    const AppendEntriesRequest& request) -> Task<AppendEntriesResponse> {
    auto member_id = transport_.member_id();
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
        co_return make_append_entries_response(
            member_id, current_term, false, logs_.last_log_index(), std::nullopt);
    }

    if (term > current_term) {
        increase_term_to(term);
        current_term = term;
    }

    leader_id_ = leader_id;
    last_reset_time_.store(util::current_ms(), std::memory_order_relaxed);
    // LOG_INFO("receive heartbeat from {}, current_term: {}", leader_id_.value(), hard_state_.current_term());

    // 判断追加日志的上一条日志是否存在与本地日志中
    bool log_ok{false};
    if (prev_log_index <= logs_.last_log_index() &&
        prev_log_index >= logs_.last_included_index() &&
        prev_log_term == logs_.get_term(prev_log_index)) {
        log_ok = true;
    }

    if (!log_ok) {
        co_return make_append_entries_response(
            member_id,  current_term, false, logs_.last_log_index(), prev_log_index);
    }

    if (!entries.empty()) {
        // 删除冲突的日志
        for (const auto& entry : entries) {
            auto idx = entry.index();
            if (idx > logs_.last_log_index()) {
                break;
            }

            if (logs_.get_term(idx) != entry.term()) {
                persister_.truncate_entries(idx, logs_.last_log_index());
                logs_.truncate_entries_before(idx);
                break;
            }
        }

        // 追加并持久化日志
        persister_.save_entries(entries);
        logs_.append_entries(entries);
    }

    if (leader_commit > commit_index_) {
        commit_index_ = std::min(logs_.last_log_index(), leader_commit);
        co_await apply_entry();
    }

    co_return make_append_entries_response(
        member_id, current_term, true, logs_.last_log_index(), std::nullopt);
}

auto vosfs::raft::RaftNode::handle_install_snapshot_request(
    const InstallSnapshotRequest& request) -> Task<InstallSnapshotResponse> {
    auto member_id = transport_.member_id();
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
        co_return make_install_snapshot_response(member_id, current_term);
    }

    if (term > current_term) {
        increase_term_to(term);
        current_term = term;
    }

    leader_id_ = leader_id;
    last_reset_time_.store(util::current_ms(), std::memory_order_relaxed);

    if (offset == 0) {
        snapshot_data_.clear();
    }

    snapshot_data_.append(data);

    if (!done) {
        co_return make_install_snapshot_response(member_id, current_term);
    }

    Snapshot snapshot;
    if (!snapshot.ParseFromString(snapshot_data_)) {
        LOG_WARN("failed to parse from snapshot data.");
        snapshot_data_.clear();
        co_return make_install_snapshot_response(member_id, current_term);
    }

    // 加载快照
    auto& inode_map = snapshot.inodes();
    std::unordered_map<uint64_t, Inode> inodes;
    for (const auto& [ino, inode] : inode_map) {
        inodes.emplace(ino, inode);
    }

    logs_.set_last_included_index(last_included_index);
    logs_.set_last_included_term(last_included_term);
    state_machine_.set_inodes(std::move(inodes));
    co_return make_install_snapshot_response(member_id, current_term);
}

auto vosfs::raft::RaftNode::handle_request_vote_response(
    const RequestVoteResponse& response) -> Task<void> {
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
        increase_term_to(term);
        co_return;
    }

    if (role_.load(std::memory_order_relaxed) == kLeader) {
        co_return;
    }

    if (vote_granted) {
        if (++votes_ > transport_.cluster_size() / 2 &&
            role_.load(std::memory_order_relaxed) == kCandidate) {
            become_leader();
        }
    }
}

auto vosfs::raft::RaftNode::handle_append_entries_response(
    const AppendEntriesResponse& response) -> Task<void> {
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
        increase_term_to(term);
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
    idxs.reserve(transport_.cluster_size());
    for (auto idx: match_index_ | std::views::values) {
        idxs.push_back(idx);
    }
    std::ranges::sort(idxs);
    if (idxs[idxs.size()/2] > commit_index_) {
        commit_index_ = idxs[idxs.size()/2];
        co_await apply_entry();
    }
}

auto vosfs::raft::RaftNode::handle_install_snapshot_response(
    const InstallSnapshotResponse& response) -> Task<void> {
    auto id = response.id();
    auto term = response.term();

    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    auto current_term = hard_state_.current_term();
    if (term > current_term) {
        increase_term_to(term);
        co_return;
    }

    co_await send_snapshot(id);
}

auto vosfs::raft::RaftNode::make_request_vote_request(
    uint64_t term,
    uint64_t candidate_id,
    uint64_t last_log_index,
    uint64_t last_log_term) -> RequestVoteRequest {
    RequestVoteRequest request;
    request.set_term(term);
    request.set_candidate_id(candidate_id);
    request.set_last_log_index(last_log_index);
    request.set_last_log_term(last_log_term);
    return request;
}

auto vosfs::raft::RaftNode::make_request_vote_response(
    uint64_t id,
    uint64_t term,
    bool vote_granted) -> RequestVoteResponse {
    RequestVoteResponse response;
    response.set_id(id);
    response.set_term(term);
    response.set_vote_granted(vote_granted);
    return response;
}

auto vosfs::raft::RaftNode::make_append_entries_request(
    uint64_t term,
    uint64_t leader_id,
    uint64_t prev_log_index,
    uint64_t prev_log_term,
    std::span<LogEntry> entries,
    uint64_t leader_commit) -> AppendEntriesRequest {
    AppendEntriesRequest request;
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

auto vosfs::raft::RaftNode::make_append_entries_response(
    uint64_t id,
    uint64_t term,
    bool success,
    uint64_t last_log_index,
    std::optional<uint64_t> conflict_index) -> AppendEntriesResponse {
    AppendEntriesResponse response;
    response.set_id(id);
    response.set_term(term);
    response.set_success(success);
    response.set_last_log_index(last_log_index);
    if (conflict_index.has_value()) {
        response.set_conflict_index(conflict_index.value());
    }
    return response;
}

auto vosfs::raft::RaftNode::make_install_snapshot_request(
    uint64_t term,
    uint64_t leader_id,
    uint64_t last_included_index,
    uint64_t last_included_term) -> InstallSnapshotRequest {
    InstallSnapshotRequest request;
    request.set_term(term);
    request.set_leader_id(leader_id);
    request.set_last_included_index(last_included_index);
    request.set_last_included_term(last_included_term);
    return request;
}

auto vosfs::raft::RaftNode::make_install_snapshot_response(
    uint64_t id,
    uint64_t term) -> InstallSnapshotResponse {
    InstallSnapshotResponse response;
    response.set_id(id);
    response.set_term(term);
    return response;
}
