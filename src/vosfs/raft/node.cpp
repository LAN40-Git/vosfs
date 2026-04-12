#include "vosfs/raft/node.hpp"

auto vosfs::raft::RaftNode::shutdown() -> kosio::async::Task<void> {
    is_shutdown_.store(true, std::memory_order_release);
    auto ret = co_await client_provider_->shutdown();
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

        do_election();
    }
}

auto vosfs::raft::RaftNode::heartbeat_loop() -> kosio::async::Task<void> {
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

        do_heartbeat();
    }
}

void vosfs::raft::RaftNode::do_election() {
    // 为自己投票
    votes_ = 1;
    voted_for_ = transport_.member_id();
    // 更新任期
    current_term_.fetch_add(1, std::memory_order_relaxed);
    // 进化为候选人
    role_.store(kCandidate, std::memory_order_relaxed);
    // 持久化
    persist_hard_state();
    // 广播选举请求
    auto request = detail::MessageFactory::make_request_vote_request(
        current_term_.load(std::memory_order_relaxed),
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

    if (++votes_ > transport_.peer_count() / 2 &&
            role_.load(std::memory_order_relaxed) == kCandidate) {
        become_leader();
    }
}

void vosfs::raft::RaftNode::do_heartbeat() {
    auto current_term = current_term_.load(std::memory_order_relaxed);
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
                // TODO: send snapshot
                continue;
            }

            auto batch_size = std::min(detail::MAX_ENTRIES_PER_APPEND, last_log_index - next_index + 1);
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

void vosfs::raft::RaftNode::increase_term_to(uint64_t term) {
    // 获得的票数设置为0
    votes_ = 0;
    // 重置投票对象
    voted_for_.reset();
    // 更新当前任期
    current_term_.store(term, std::memory_order_release);
    // 退化为追随者
    role_.store(kFollower, std::memory_order_release);
    // 持久化
    persist_hard_state();
}

void vosfs::raft::RaftNode::become_leader() {
    votes_ = 0;
    voted_for_.reset();
    role_.store(kLeader, std::memory_order_release);
    persist_hard_state();
    leader_id_ = transport_.member_id();
    // 更新每个Raft节点的 next_index
    auto& peers = transport_.peers();
    for (const auto& peer : peers | std::views::values) {
        next_index_[peer.member_id()] = logs_.last_log_index() + 1;
    }
}

void vosfs::raft::RaftNode::apply_to_state_machine() {
    auto size = commit_index_ - last_applied_;
    state_machine_.apply(logs_.get_entries_span(last_applied_ + 1, size));
    last_applied_ = commit_index_;
}

void vosfs::raft::RaftNode::persist_hard_state() {
    hard_state_.set_current_term(current_term_.load(std::memory_order_relaxed));
    if (voted_for_.has_value()) {
        hard_state_.set_voted_for(voted_for_.value());
    } else {
        hard_state_.clear_voted_for();
    }
    hard_state_.set_commit_index(commit_index_);
    auto ret = persister_.persist(detail::HARD_STATE_KEY, hard_state_.SerializeAsString());
    if (!ret) {
        LOG_FATAL("persist hard state failed : {}", ret.error());
        // TODO: 关闭节点，标记为宕机，此时无法接收消息也无法发送消息
    }
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

    auto current_term = current_term_.load(std::memory_order_relaxed);
    if (term < current_term) {
        co_return detail::MessageFactory::make_request_vote_response(resp_payload, transport_.member_id(), current_term, false);
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

    bool can_vote = (!voted_for_.has_value() || voted_for_ == candidate_id) && up_to_date_log;

    if (can_vote) {
        voted_for_ = candidate_id;
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

    auto current_term = current_term_.load(std::memory_order_relaxed);
    if (term < current_term) {
        co_return detail::MessageFactory::make_append_entries_response(
            resp_payload, transport_.member_id(), current_term, false, logs_.last_log_index());
    }

    if (term > current_term) {
        increase_term_to(term);
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
                auto ret = logs_.truncate_entries(idx);
                if (!ret) {
                    LOG_WARN("{}", ret.error());
                    co_return detail::MessageFactory::make_append_entries_response(
                        resp_payload, transport_.member_id(), current_term, false, logs_.last_log_index());
                }
                break;
            }
        }

        // 追加并持久化日志
        auto ret = logs_.append_entries(entries);
        if (!ret) {
            LOG_WARN("{}", ret.error());
            co_return detail::MessageFactory::make_append_entries_response(
                resp_payload, transport_.member_id(), current_term, false, logs_.last_log_index());
        }
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

    // TODO: 处理快照请求
}

auto vosfs::raft::RaftNode::handle_request_vote_response(std::string_view resp_payload) -> kosio::async::Task<void> {
    RequestVoteResponse response;
    if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
        co_return;
    }

    [[maybe_unused]] auto id = response.id();
    auto term = response.term();
    auto vote_granted = response.vote_granted();

    if (term < current_term_.load(std::memory_order_acquire)) {
        co_return;
    }

    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    auto current_term = current_term_.load(std::memory_order_relaxed);
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
        if (++votes_ > transport_.peer_count() / 2 &&
            role_.load(std::memory_order_relaxed) == kCandidate) {
            become_leader();
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

    if (term < current_term_.load(std::memory_order_acquire) ||
        role_.load(std::memory_order_acquire) != kLeader) {
        co_return;
    }

    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    auto current_term = current_term_.load(std::memory_order_relaxed);
    if (term < current_term ||
        role_.load(std::memory_order_relaxed) != kLeader) {
        co_return;
    }

    if (term > current_term) {
        increase_term_to(term);
        current_term = term;
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
    commit_index_ = idxs[idxs.size()/2];
    apply_to_state_machine();
}

auto vosfs::raft::RaftNode::handle_install_snapshot_response(std::string_view resp_payload) -> kosio::async::Task<void> {
    InstallSnapshotResponse response;
    if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
        LOG_ERROR("Failed to parse request vote response");
        co_return;
    }

    // TODO: 处理快照回复
}
