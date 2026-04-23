#include <kosio/signal/signal.hpp>
#include "vosfs/auth/auth_server.hpp"
using namespace vosfs::auth;

auto main() -> int {
    SET_LOG_LEVEL(kosio::log::LogLevel::Verbose);
    auto has_auth_server = AuthServer::create("test.db", 8080);
    if (!has_auth_server) {
        LOG_ERROR("{}", has_auth_server.error());
        return -1;
    }
    auto auth_server = std::move(has_auth_server.value());
    auth_server->wait();
}
