#include <kosio/signal.hpp>
#include "vosfs/api/mathpb/math.pb.h"
#include "vosfs/rpc/consumer.hpp"

using namespace vosfs;
using namespace vosfs::rpc;

auto process(std::unique_ptr<RpcConsumer> consumer) -> kosio::async::Task<void> {
    while (true) {
        co_await kosio::time::sleep(1000);

        math::MathRequest request;
        request.set_a(20);
        request.set_b(10);
        auto ret = co_await consumer->send_request(ServiceType::kMath, MethodType::kMathAdd, request.SerializeAsString(),
            [](std::string_view resp_payload) -> kosio::async::Task<void> {
            math::MathResponse response;
            if (!response.ParseFromArray(resp_payload.data(), resp_payload.size())) {
                LOG_ERROR("Failed to parse rpc response");
                co_return;
            }

            LOG_INFO("Result : {}", response.result());
        });
        if (!ret) {
            LOG_ERROR("Failed to send request : {}", ret.error());
            break;
        }
    }

    // co_await consumer->shutdown();
}

auto main_loop() -> kosio::async::Task<void> {
    auto has_consumer = co_await RpcConsumer::create("127.0.0.1", 8080);
    if (!has_consumer) {
        LOG_ERROR("Failed to create consumer : {}", has_consumer.error());
        co_return;
    }

    co_await process(std::move(has_consumer.value()));
}

auto main() -> int {
    SET_LOG_LEVEL(kosio::log::LogLevel::Verbose);
    kosio::runtime::MultiThreadBuilder::options().set_num_workers(4).build().block_on(main_loop());
}