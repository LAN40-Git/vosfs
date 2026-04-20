#include <kosio/signal/signal.hpp>
#include "vosfs-auth/server.hpp"
using namespace vosfs::auth;
auto shutdown_handler(std::unique_ptr<Server>& server, uint64_t timeout = 0) -> kosio::async::Task<void> {
    if (timeout == 0) {
        co_await kosio::signal::ctrl_c();
    } else {
        co_await kosio::time::sleep(timeout);
    }
    co_await server->shutdown();
}

auto main_loop() -> kosio::async::Task<void> {
    auto has_auth_server = Server::create("test", 8080);
    if (!has_auth_server) {
        LOG_ERROR("{}", has_auth_server.error());
        co_return;
    }

    auto auth_server = std::move(has_auth_server.value());
    kosio::spawn(shutdown_handler(auth_server));
    co_await auth_server->wait();
}

auto main() -> int {
    SET_LOG_LEVEL(kosio::log::LogLevel::Verbose);
    kosio::runtime::CurrentThreadBuilder::default_create().block_on(main_loop());
}
