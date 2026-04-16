#include "vosfs/raft/server.hpp"

auto server() -> kosio::async::Task<void> {
    auto has_meta_server = co_await vosfs::raft::MetaServer::create("vosfs");
    if (!has_meta_server) {
        LOG_ERROR("{}", has_meta_server.error());
        co_return;
    }

    auto meta_server = std::move(has_meta_server.value());
    co_await meta_server->run();
}

auto main() -> int {
    SET_LOG_LEVEL(kosio::log::LogLevel::Verbose);
    kosio::runtime::CurrentThreadBuilder::default_create().block_on(server());
}