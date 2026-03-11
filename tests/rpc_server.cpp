#include <kosio/signal.hpp>
#include "vosfs/rpc/provider.hpp"

using namespace vosfs;
using namespace vosfs::rpc;

auto shutdown(std::unique_ptr<RpcProvider>& provider) -> kosio::async::Task<void> {
    co_await kosio::time::sleep(5000);
    auto ret = co_await provider->shutdown();
    if (!ret) {
        LOG_ERROR("Failed to shutdown provider  : {}", ret.error());
    }
}

auto process(std::unique_ptr<RpcProvider>& provider) -> kosio::async::Task<void> {
    auto ret = co_await provider->run();
    if (!ret) {
        LOG_ERROR("Failed to run provider : {}", ret.error());
    }

    LOG_INFO("Provider stopped.");
}

auto main_loop() -> kosio::async::Task<void> {
    auto has_provider = co_await RpcProvider::create(8080);
    if (!has_provider) {
        LOG_ERROR("Failed to create RpcProvider : {}", has_provider.error());
        co_return;
    }

    auto provider = std::move(has_provider.value());
    // Register invokes
    provider->register_invoke(ServiceType::kMath, MethodType::kMathAdd, [](
        std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<Result<std::size_t>> {
        math::MathRequest request;
        math::MathResponse response;

        if (!request.ParseFromArray(req_payload.data(), req_payload.size())) {
            co_return std::unexpected{make_error(Error::kMessageParseFailed)};
        }

        response.set_result(request.a() + request.b());
        if (!response.SerializeToArray(resp_payload.data(), resp_payload.size())) {
            co_return std::unexpected{make_error(Error::kMessageSerializeFailed)};
        }

        co_return response.ByteSizeLong();
    });

    provider->register_invoke(ServiceType::kMath, MethodType::kMathSub, [](
        std::string_view req_payload, std::span<char> resp_payload) -> kosio::async::Task<Result<std::size_t>> {
        math::MathRequest request;
        math::MathResponse response;

        if (!request.ParseFromArray(req_payload.data(), req_payload.size())) {
            co_return std::unexpected{make_error(Error::kMessageParseFailed)};
        }

        response.set_result(request.a() - request.b());
        if (!response.SerializeToArray(resp_payload.data(), resp_payload.size())) {
            co_return std::unexpected{make_error(Error::kMessageSerializeFailed)};
        }

        co_return response.ByteSizeLong();
    });

    kosio::spawn(process(provider));
    co_await kosio::signal::ctrl_c();
    co_await provider->shutdown();
}

auto main() -> int {
    SET_LOG_LEVEL(kosio::log::LogLevel::Verbose);
    kosio::runtime::MultiThreadBuilder::options().set_num_workers(4).build().block_on(main_loop());
}