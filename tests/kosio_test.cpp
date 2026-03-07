#include <kosio/core.hpp>
#include <kosio/net.hpp>

using namespace kosio;
using namespace kosio::net;
using namespace kosio::async;

auto test(int fd) -> Task<void> {
    co_await time::sleep(5000);
    auto has_cancle = co_await io::cancel(fd, IORING_ASYNC_CANCEL_ALL);
    if (!has_cancle) {
        LOG_ERROR("{}", has_cancle.error());
    }
}

auto process(TcpStream stream) -> Task<void> {
    char buf[1024]{};
    spawn(test(stream.fd()));
    while (true) {
        auto has_len = co_await stream.read(buf);
        if (!has_len) {
            LOG_ERROR("{}", has_len.error());
            break;
        }
        auto len = has_len.value();
        if (len == 0) {
            LOG_INFO("Connection from {} closed", stream.peer_addr().value());
            break;
        }
        co_await stream.write_all({buf, len});
    }
    LOG_INFO("Connection from {} closed", stream.peer_addr().value());
}

auto server() -> Task<void> {
    auto addr = SocketAddr::parse("localhost", 8080).value();
    auto listener = TcpListener::bind(addr).value();
    while (true) {
        auto [stream, addr] = (co_await listener.accept()).value();
        LOG_INFO("Accepted connection from {}", addr);
        spawn(process(std::move(stream)));
    }
}

auto main() -> int {
    SET_LOG_LEVEL(log::LogLevel::Debug);
    kosio::runtime::MultiThreadBuilder::default_create().block_on(server());
}