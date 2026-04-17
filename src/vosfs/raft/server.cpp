#include "vosfs/raft/server.hpp"
#include "vosfs/common/util/message_factory.hpp"
#include <ranges>
#include <kosio/signal/signal.hpp>

vosfs::raft::RaftNode::RaftNode(
    rpc::RpcServer raft_rpc_server,
    rpc::RpcServer client_rpc_server,
    Persister&& persister,
    detail::RaftLog&& logs,
    detail::Transport&& transport,
    DataClusterInfo&& data_cluster_info,
    HardState&& hard_state)
    : raft_rpc_server_(std::move(raft_rpc_server))
    , client_rpc_server_(std::move(client_rpc_server))
    , persister_(std::move(persister))
    , logs_(std::move(logs))
    , transport_(std::move(transport))
    , data_cluster_info_(std::move(data_cluster_info))
    , hard_state_(std::move(hard_state)) {}

auto vosfs::raft::RaftNode::create(std::string_view data_dir) -> kosio::async::Task<Result<std::unique_ptr<RaftNode>>> {
    // 创建 Raft RPC 服务层
    auto has_raft_rpc_server = co_await rpc::RpcProvider::create(detail::RAFT_RPC_PORT);
    if (!has_raft_rpc_server) {
        co_return std::unexpected{has_raft_rpc_server.error()};
    }
    auto raft_rpc_server = std::move(has_raft_rpc_server.value());

    // 创建 Client RPC 服务层
    auto has_client_rpc_server = co_await rpc::RpcProvider::create(detail::CLIENT_RPC_PORT);
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
    auto transport = std::move(has_transport.value());

    // 加载数据节点信息
    auto has_data_cluster_info = persister.load_data_cluster_info();
    if (!has_data_cluster_info) {
        co_return std::unexpected{has_data_cluster_info.error()};
    }
    auto data_cluster_info = std::move(has_data_cluster_info.value());

    // 加载 HardState
    auto has_hard_state = persister.load_hard_state();
    if (!has_hard_state) {
        co_return std::unexpected{has_hard_state.error()};
    }
    auto hard_state = std::move(has_hard_state.value());

    // 创建节点
    co_return std::unique_ptr<RaftNode>(new RaftNode{
        std::move(raft_rpc_server),
        std::move(client_rpc_server),
        std::move(persister),
        std::move(logs),
        std::move(transport),
        std::move(data_cluster_info),
        std::move(hard_state)});
}

auto vosfs::raft::RaftNode::run() -> kosio::async::Task<void> {
    co_await init();
    kosio::spawn(raft_rpc_server_->run());
    kosio::spawn(client_rpc_server_->run());
    kosio::spawn(election_loop());
    kosio::spawn(heartbeat_loop());
    co_await kosio::signal::ctrl_c();
    co_await shutdown();
}

auto vosfs::raft::RaftNode::shutdown() -> kosio::async::Task<void> {
    {
        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);
        co_await transport_.shutdown();
        co_await raft_rpc_server_->shutdown();
        co_await client_rpc_server_->shutdown();
    }
    is_shutdown_.store(true, std::memory_order_release);
    co_await latch_.wait();
}

auto vosfs::raft::RaftNode::init() -> kosio::async::Task<void> {
    // 尝试加载快照
    if (auto has_snapshot = persister_.load_snapshot(); has_snapshot) {
        auto snapshot = std::move(has_snapshot.value());
        co_await apply_snapshot(snapshot);
    }

    // 注册 RPC 服务
    using rpc::ServiceType;
    using rpc::MethodType;
    // ========== Raft RPC ==========
    // 选举
    raft_rpc_server_->register_handler(ServiceType::kRaft, MethodType::kRaftRequestVote,
        [this](std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
            co_return co_await this->handle_request_vote_request(req_payload, resp_payload);
        });

    // 日志同步
    raft_rpc_server_->register_handler(ServiceType::kRaft, MethodType::kRaftAppendEntries,
        [this](std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
            co_return co_await this->handle_append_entries_request(req_payload, resp_payload);
        });

    // 快照安装
    raft_rpc_server_->register_handler(ServiceType::kRaft, MethodType::kRaftInstallSnapshot,
        [this](std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
            co_return co_await this->handle_install_snapshot_request(req_payload, resp_payload);
        });

    // ========== Client RPC ==========

}

auto vosfs::raft::RaftNode::apply_snapshot(Snapshot& snapshot) -> kosio::async::Task<void> {
    // 缓存新快照
    snapshot_data_ = snapshot.SerializeAsString();
    // 持久层持久化快照数据和元数据
    persister_.save_snapshot(snapshot);
    // 状态机应用快照-更新 Inode 信息
    state_machine_.apply_snapshot(snapshot);
    // 日志层应用快照-更新快照元数据
    logs_.apply_snapshot(snapshot);
    // 通信层应用快照-集群配置
    co_await transport_.apply_snapshot(snapshot);
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

        // 广播选举请求
        auto request = util::MessageFactory::make_request_vote_request(
            current_term,
            member_id,
            last_log_index,
            last_log_term);

        co_await transport_.broadcast_request(
            rpc::ServiceType::kRaft,
            rpc::MethodType::kRaftRequestVote,
            request,
            [this](std::string_view resp_payload) -> kosio::async::Task<void> {
                co_await this->handle_request_vote_response(resp_payload);
            });
    }
    latch_.count_down();
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
                    // 若该节点此时未接收快照，则开始发送快照
                    if (snapshot_context_[member_id] == 0 && !snapshot_data_.empty()) {
                        co_await send_snapshot(member_id, 0);
                    }
                    continue;
                }

                auto batch_size = std::min(detail::MAX_ENTRIES_PER_APPEND, last_log_index - next_index + 1);
                entries = logs_.get_entries(next_index, batch_size);
            }

            auto prev_log_index = next_index - 1;
            auto prev_log_term = logs_.get_term(prev_log_index);

            requests.emplace(member_id, util::MessageFactory::make_append_entries_request(
                current_term,
                leader_id,
                prev_log_index,
                prev_log_term,
                entries,
                commit_index));
        }

        // 发送日志同步请求（心跳）
        for (const auto& [member_id, request] : requests) {
            co_await transport_.unicast_request(
                member_id,
                rpc::ServiceType::kRaft,
                rpc::MethodType::kRaftAppendEntries,
                request,
                [this](std::string_view resp_payload) -> kosio::async::Task<void> {
                    co_await this->handle_append_entries_response(resp_payload);
                });
        }
    }
    latch_.count_down();
}

auto vosfs::raft::RaftNode::send_snapshot(uint64_t member_id, uint64_t offset) -> kosio::async::Task<void> {
    if (snapshot_data_.empty()) {
        co_return;
    }

    auto current_term = hard_state_.current_term();
    auto leader_id = transport_.member_id();
    auto last_included_index = logs_.last_included_index();
    auto last_included_term = logs_.last_included_term();
    auto size = std::min(detail::MAX_SNAPSHOT_CHUNK_SIZE, snapshot_data_.size() - offset);
    auto data = snapshot_data_.substr(offset, size);
    snapshot_context_[member_id] += size;
    auto done = snapshot_context_[member_id] >= snapshot_data_.size();

    // 发送快照安装请求
    auto request = util::MessageFactory::make_install_snapshot_request(
        current_term,
        leader_id,
        last_included_index,
        last_included_term,
        offset,
        std::move(data),
        done);

    co_await transport_.unicast_request(
        member_id,
        rpc::ServiceType::kRaft,
        rpc::MethodType::kRaftInstallSnapshot,
        request,
        [this](std::string_view resp_payload) -> kosio::async::Task<void> {
            co_await this->handle_install_snapshot_response(resp_payload);
        });
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
    LOG_VERBOSE("become leader, current_term: {}", hard_state_.current_term());
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
        if (last_applied_ % detail::SNAPSHOT_INTERVAL == 0) {
            // TODO: 创建快照

        }
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

    auto current_term = hard_state_.current_term();
    if (term < current_term) {
        co_return util::MessageFactory::make_request_vote_response(resp_payload, transport_.member_id(), current_term, false);
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

    co_return util::MessageFactory::make_request_vote_response(resp_payload, transport_.member_id(), current_term, can_vote);
}

auto vosfs::raft::RaftNode::handle_append_entries_request(
    std::string_view req_payload,
    std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
    AppendEntriesRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
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
        co_return util::MessageFactory::make_append_entries_response(
            resp_payload, transport_.member_id(), current_term, false, logs_.last_log_index());
    }

    if (term > current_term) {
        increase_term_to(term);
        current_term = term;
    }

    leader_id_ = leader_id;
    last_reset_time_.store(kosio::util::current_ms(), std::memory_order_relaxed);
    LOG_VERBOSE("receive heartbeat from {}, current_term: {}", leader_id_.value(), hard_state_.current_term());

    // 判断追加日志的上一条日志是否存在与本地日志中
    bool log_ok{false};
    if (prev_log_index <= logs_.last_log_index() &&
        prev_log_index >= logs_.last_included_index() &&
        prev_log_term == logs_.get_term(prev_log_index)) {
        log_ok = true;
    }

    if (!log_ok) {
        co_return util::MessageFactory::make_append_entries_response(
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
        apply_to_state_machine();
    }

    co_return util::MessageFactory::make_append_entries_response(
        resp_payload, transport_.member_id(), current_term, true, logs_.last_log_index());
}

auto vosfs::raft::RaftNode::handle_install_snapshot_request(
    std::string_view req_payload,
    std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
    InstallSnapshotRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
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
        co_return util::MessageFactory::make_install_snapshot_response(resp_payload, current_term);
    }

    if (term > current_term) {
        increase_term_to(term);
        current_term = term;
    }

    leader_id_ = leader_id;
    last_reset_time_.store(kosio::util::current_ms(), std::memory_order_relaxed);

    if (offset == 0) {
        snapshot_data_.clear();
    }

    snapshot_data_.append(data);

    if (!done) {
        co_return util::MessageFactory::make_install_snapshot_response(resp_payload, current_term);
    }

    Snapshot snapshot;
    if (!snapshot.ParseFromString(snapshot_data_)) {
        LOG_WARN("failed to parse from snapshot data.");
        snapshot_data_.clear();
        co_return util::MessageFactory::make_install_snapshot_response(resp_payload, current_term);
    }

    co_await apply_snapshot(snapshot);
    co_return util::MessageFactory::make_install_snapshot_response(resp_payload, current_term);
}

auto vosfs::raft::RaftNode::handle_transmit_file_request(
    std::string_view req_payload,
    std::span<char> resp_payload) -> kosio::async::Task<rpc::RpcResult> {
    thread_local kosio::util::FastRand rand;

    TransmitFileRequest request;
    if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
        co_return rpc::make_result(rpc::RpcResult::kMessageParseFailed);
    }

    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    if (role_ != kLeader) {
        co_return util::MessageFactory::make_transmit_file_response(resp_payload, false, "", nullptr);
    }

    /*
     * 1. 获取分片存储的目录
     * 2. 获取分片存储的地址
     */

    std::filesystem::path path{};
    std::deque<std::string> name_queue{};
    auto parent_ino = request.parent_ino();

    const auto& inodes = state_machine_.inodes();
    while (true) {
        auto it = inodes.find(parent_ino);
        if (it == inodes.end()) {
            break;
        }
        auto& inode = it->second;
        name_queue.push_front(inode.name());
        parent_ino = inode.parent_ino();
        if (parent_ino == 0) {
            break;
        }
    }

    if (name_queue.empty()) {
        co_return util::MessageFactory::make_transmit_file_response(resp_payload, false, "", nullptr);
    }

    while (!name_queue.empty()) {
        path /= name_queue.front();
        name_queue.pop_front();
    }

    auto* chunk_metadata = request.mutable_chunk_metadata();
    auto& data_node_infos = data_cluster_info_.data_node_infos();
    auto mod = data_cluster_info_.data_node_infos().size();
    auto n = rand.fastrand_n(UINT32_MAX);
    for (auto& metadata : *chunk_metadata) {
        auto id = metadata.id();
        auto index = std::hash<uint64_t>{}(n+id) % mod;
        metadata.set_ip(data_node_infos.at(static_cast<int>(index)).ip());
        metadata.set_port(data_node_infos.at(static_cast<int>(index)).port());
    }

    co_return util::MessageFactory::make_transmit_file_response(resp_payload, true, path.string(), chunk_metadata);
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

auto vosfs::raft::RaftNode::handle_append_entries_response(std::string_view resp_payload) -> kosio::async::Task<void> {
    AppendEntriesResponse response;
    if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
        LOG_WARN("failed to parse request vote response");
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
        apply_to_state_machine();
    }
}

auto vosfs::raft::RaftNode::handle_install_snapshot_response(std::string_view resp_payload) -> kosio::async::Task<void> {
    InstallSnapshotResponse response;
    if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
        LOG_WARN("failed to parse request vote response");
        co_return;
    }

    auto id = response.id();
    auto term = response.term();

    co_await mutex_.lock();
    std::lock_guard lock(mutex_, std::adopt_lock);

    auto current_term = hard_state_.current_term();
    if (term > current_term) {
        snapshot_context_[id] = 0;
        increase_term_to(term);
        co_return;
    }

    auto offset = snapshot_context_[id];

    // 检查快照是否传送完毕
    if (offset >= snapshot_data_.size()) {
        snapshot_context_[id] = 0;
        co_return;
    }

    co_await send_snapshot(id, offset);
}
