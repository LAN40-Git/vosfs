#include <kosio/signal/signal.hpp>
#include "vosfs/auth/server.hpp"

auto server() -> kosio::async::Task<void> {
    auto has_auth_server = co_await vosfs::auth::AuthServer::create(8080);
    if (!has_auth_server) {
        LOG_ERROR("{}", has_auth_server.error());
        co_return;
    }

    auto auth_server = std::move(has_auth_server.value());
    kosio::spawn(auth_server.run());
    co_await kosio::signal::ctrl_c();
    co_await auth_server.shutdown();
}

auto main() -> int {
    SET_LOG_LEVEL(kosio::log::LogLevel::Verbose);
    kosio::runtime::CurrentThreadBuilder::default_create().block_on(server());
}
