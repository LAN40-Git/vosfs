#include "vosfs/raft/internal/transport.hpp"

vosfs::raft::detail::Transport::Transport(
    RaftCluster&& cluster,
    std::unique_ptr<rpc::RpcProvider> raft_provider,
    std::unique_ptr<rpc::RpcProvider> client_provider)
    : cluster_(std::move(cluster))
    , raft_provider_(std::move(raft_provider))
    , client_provider_(std::move(client_provider)) {}


auto vosfs::raft::detail::Transport::create(RaftCluster&& cluster) -> kosio::async::Task<Result<Transport>> {
    // create raft provider
    auto ret = co_await rpc::RpcProvider::create(RAFT_PROVIDER_PORT, rpc::RpcProvider::AuthMode::NONE);
    if (!ret) {
        LOG_ERROR("{}", ret.error());
        co_return std::unexpected{make_error(Error::kCreateProviderFailed)};
    }
    auto raft_provider = std::move(ret.value());
    // create client provider
    ret = co_await rpc::RpcProvider::create(CLIENT_PROVIDER_PORT, rpc::RpcProvider::AuthMode::REQUIRED);
    if (!ret) {
        LOG_ERROR("{}", ret.error());
        co_return std::unexpected{make_error(Error::kCreateProviderFailed)};
    }
    auto client_provider = std::move(ret.value());
    co_return Transport(std::move(cluster), std::move(raft_provider), std::move(client_provider));
}

