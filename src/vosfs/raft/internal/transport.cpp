#include "vosfs/raft/internal/transport.hpp"

auto vosfs::raft::detail::Transport::create(RaftCluster&& cluster) -> kosio::async::Task<Result<Transport>> {
    // // create raft provider
    // auto ret = co_await rpc::RpcProvider::create(RAFT_PROVIDER_PORT, rpc::RpcProvider::AuthMode::NONE);
    // if (!ret) {
    //     LOG_ERROR("{}", ret.error());
    //     co_return std::unexpected{make_error(Error::kCreateProviderFailed)};
    // }
    // auto raft_provider = std::move(ret.value());
    // // create client provider
    // ret = co_await rpc::RpcProvider::create(CLIENT_PROVIDER_PORT, rpc::RpcProvider::AuthMode::REQUIRED);
    // if (!ret) {
    //     LOG_ERROR("{}", ret.error());
    //     co_return std::unexpected{make_error(Error::kCreateProviderFailed)};
    // }
    // auto client_provider = std::move(ret.value());
    co_return Transport(std::move(cluster));
}

// void vosfs::raft::detail::Transport::run() const {
//     kosio::spawn(raft_provider_->run());
//     kosio::spawn(client_provider_->run());
// }
//
// auto vosfs::raft::detail::Transport::shutdown() const -> kosio::async::Task<Result<void>> {
//     auto ret = co_await raft_provider_->shutdown();
//     if (!ret) {
//         co_return ret;
//     }
//     ret = co_await client_provider_->shutdown();
//     co_return ret;
// }

auto vosfs::raft::detail::Transport::unicast_request(
    uint64_t peer_id, rpc::ServiceType service_type,
    rpc::MethodType method_type, std::string_view req_payload, const rpc::RpcCallback& callback)
    -> kosio::async::Task<void> {
    auto& peers = cluster_.peers();
    auto it = peers.find(peer_id);
    if (it == peers.end()) {
        LOG_ERROR("peer {} not found", peer_id);
        co_return;
    }

    auto& peer = it->second;
    if (auto ret = co_await peer.check_status(); !ret) {
        LOG_ERROR("{}", ret.error());
        co_return;
    }

    if (auto ret = co_await peer.send_request(service_type, method_type, req_payload, callback)) {
        LOG_ERROR("{}", ret.error());
        co_return;
    }
}

auto vosfs::raft::detail::Transport::unicast_request(uint64_t peer_id, rpc::ServiceType service_type,
    rpc::MethodType method_type, std::string&& req_payload, rpc::RpcCallback&& callback)
    -> kosio::async::Task<void> {
    auto& peers = cluster_.peers();
    auto it = peers.find(peer_id);
    if (it == peers.end()) {
        LOG_ERROR("peer {} not found", peer_id);
        co_return;
    }

    auto& peer = it->second;
    if (auto ret = co_await peer.check_status(); !ret) {
        LOG_ERROR("{}", ret.error());
        co_return;
    }

    if (auto ret = co_await peer.send_request(service_type, method_type, std::move(req_payload), callback)) {
        LOG_ERROR("{}", ret.error());
    }
}

auto vosfs::raft::detail::Transport::broadcast_request(rpc::ServiceType service_type, rpc::MethodType method_type,
    std::string_view req_payload, const rpc::RpcCallback& callback)
    -> kosio::async::Task<void> {
    auto& peers = cluster_.peers();
    for (auto& peer : peers | std::views::values) {
        if (auto ret = co_await peer.check_status(); !ret) {
            LOG_ERROR("{}", ret.error());
            continue;
        }

        if (auto ret = co_await peer.send_request(service_type, method_type, req_payload, callback)) {
            LOG_ERROR("{}", ret.error());
        }
    }
}

auto vosfs::raft::detail::Transport::broadcast_request(
    rpc::ServiceType service_type, rpc::MethodType method_type,
    std::string&& req_payload, rpc::RpcCallback&& callback)
    -> kosio::async::Task<void> {
    auto& peers = cluster_.peers();
    for (auto& peer : peers | std::views::values) {
        if (auto ret = co_await peer.check_status(); !ret) {
            LOG_ERROR("{}", ret.error());
            continue;
        }

        if (auto ret = co_await peer.send_request(service_type, method_type, std::move(req_payload), callback)) {
            LOG_ERROR("{}", ret.error());
        }
    }
}

