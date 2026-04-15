#include <kosio/signal/signal.hpp>

#include "auth.pb.h"
#include "vosfs/rpc.hpp"

using namespace vosfs;
using namespace vosfs::rpc;
using namespace vosfs::auth;

auto process(std::unique_ptr<RpcConsumer> consumer) -> kosio::async::Task<void> {
    co_await kosio::time::sleep(1000);
    PutUserRequest request;
    request.set_name("lan");
    request.set_hashed_password("123456");
    request.set_role(Role::kUser);
    co_await consumer->send_request(ServiceType::kAuth, MethodType::kAuthPutUser, request.SerializeAsString(),
        [](std::string_view resp_payload) -> kosio::async::Task<void> {
            PutUserResponse response;
            if (!response.ParseFromArray(resp_payload.data(), static_cast<int>(resp_payload.size()))) {
                LOG_ERROR("failed to parse rpc response");
                co_return;
            }

            auto success = response.success();
            auto& msg = response.msg();

            LOG_INFO("{}", msg);
        });
    co_await kosio::signal::ctrl_c();
    co_await consumer->shutdown();
}

auto main_loop() -> kosio::async::Task<void> {
    auto has_consumer = co_await RpcConsumer::create("127.0.0.1", 8080);
    if (!has_consumer) {
        LOG_ERROR("failed to create consumer: {}", has_consumer.error());
        co_return;
    }

    co_await process(std::move(has_consumer.value()));
}

auto main() -> int {
    SET_LOG_LEVEL(kosio::log::LogLevel::Verbose);
    kosio::runtime::MultiThreadBuilder::options().set_num_workers(4).build().block_on(main_loop());
}
