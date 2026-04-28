#include <kosio/signal/signal.hpp>
#include "vosfs/auth/auth_server.hpp"
using namespace vosfs::auth;

auto main_coro() -> kosio::async::Task<void> {
    auto has_auth_server = AuthServer::create("vosfs-auth.db", "0.0.0.0", 9000);
    if (!has_auth_server) {
        LOG_ERROR("{}", has_auth_server.error());
        co_return;
    }
    auto auth_server = std::move(has_auth_server.value());
    co_await auth_server->wait();
}

auto main() -> int {
    SET_LOG_LEVEL(kosio::log::LogLevel::Verbose);
    kosio::runtime::MultiThreadBuilder::default_create().block_on(main_coro());
}
