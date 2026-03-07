#include "vosfs/rpc/provider.hpp"

using namespace vosfs;
using namespace vosfs::rpc;



auto main_loop() -> kosio::async::Task<void> {
    auto has_provider = co_await RpcProvider::create(8080);
    if (!has_provider) {
        LOG_ERROR("Failed to create RpcProvider : {}", has_provider.error());
        co_return;
    }

    auto provider = std::move(has_provider.value());
    // Register invokes
    provider->register_invoke(ServiceType::kMath, MethodType::kMathAdd, [](
        std::string_view req_payload, std::span<char> resp_payload,
        uint64_t session_id, uint64_t request_id) -> kosio::async::Task<Result<std::size_t>> {
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
        std::string_view req_payload, std::span<char> resp_payload,
        uint64_t session_id, uint64_t request_id) -> kosio::async::Task<Result<std::size_t>> {
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
}

auto main() -> int {
    return 0;
}