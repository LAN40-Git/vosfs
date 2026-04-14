#include <kosio/signal.hpp>
#include "vosfs/api/mathpb/math.pb.h"
#include "vosfs/rpc.hpp"

using namespace vosfs;
using namespace vosfs::rpc;

auto shutdown(std::unique_ptr<RpcProvider>& provider) -> kosio::async::Task<void> {
    co_await kosio::time::sleep(15000);
    co_await provider->shutdown();
}

auto process(std::unique_ptr<RpcProvider>& provider) -> kosio::async::Task<void> {
    co_await provider->run();
}

auto main_loop() -> kosio::async::Task<void> {
    auto has_provider = co_await RpcProvider::create(8080);
    if (!has_provider) {
        LOG_ERROR("Failed to create RpcProvider: {}", has_provider.error());
        co_return;
    }

    auto provider = std::move(has_provider.value());
    // Register invokes
    provider->register_handler(ServiceType::kMath, MethodType::kMathAdd, [](
        std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<RpcResult> {
        math::MathRequest request;
        math::MathResponse response;

        if (!request.ParseFromArray(req_payload.data(), static_cast<int>(req_payload.size()))) {
            co_return make_result(RpcResult::kMessageParseFailed);
        }
        response.set_result(request.a() + request.b());
        co_return serialize_response(response, resp_payload);
    });

    kosio::spawn(process(provider));
    co_await shutdown(provider);
}

auto main() -> int {
    SET_LOG_LEVEL(kosio::log::LogLevel::Verbose);
    kosio::runtime::MultiThreadBuilder::options().set_num_workers(4).build().block_on(main_loop());
}