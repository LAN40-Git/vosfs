#include <kosio/core.hpp>
#include <kosio/net.hpp>
#include <kosio/signal.hpp>

using namespace kosio;
using namespace kosio::net;
using namespace kosio::async;

auto process(TcpStream stream) -> Task<void> {
    char buf[1024]{};
    while (true) {
        std::cin >> buf;
        auto len = strlen(buf);
        co_await stream.write_all({buf, len});
        auto has_len = (co_await (stream.read_exact({buf, 2})));
        if (!has_len) {
            LOG_INFO("Connection from server has closed");
            break;
        }
        std::cout << buf << std::endl;
    }
}

auto client() -> Task<void> {
    auto addr = SocketAddr::parse("localhost", 8080).value();
    auto has_stream = co_await TcpStream::connect(addr);
    spawn(process(std::move(has_stream.value())));
    co_await kosio::signal::ctrl_c();
}

auto main() -> int {
    SET_LOG_LEVEL(log::LogLevel::Debug);
    kosio::runtime::MultiThreadBuilder::default_create().block_on(client());
}