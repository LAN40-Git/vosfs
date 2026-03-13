#pragma once
#include "vosfs/common/error.hpp"
#include <string_view>
#include <cstdint>

namespace vosfs::rpc::detail {
class RpcError {
public:
    enum ErrorCode : uint8_t {
        kSuccess = 0,
        kRedirect,
        kShutdown,
        kNeedShutdown,
        kFindServiceTypeFailed,
        kFindMethodTypeFailed,
        kGetRespPayloadFailed,
    };

public:
    explicit RpcError(uint8_t error_code)
        : error_code_(error_code) {}

public:
    [[nodiscard]]
    auto value() const noexcept -> uint8_t { return error_code_; }

    [[nodiscard]]
    auto message() const noexcept -> std::string_view {
        switch (error_code_) {
        case kSuccess:
            return "Success to handle rpc request.";
        case kRedirect:
            return "Need to redirect to another rpc provider.";
        case kShutdown:
            return "Normal shutdown.";
        case kNeedShutdown:
            return "Need to send the shutdown rp.";
        case kFindServiceTypeFailed:
            return "Failed to find service type.";
        case kFindMethodTypeFailed:
            return "Failed to find method type.";
        default:
            return "Unknown rpc error.";
        }
    }

private:
    uint8_t error_code_;
};

static auto make_rpc_error(uint8_t error_code) -> RpcError {
    return RpcError{error_code};
}

} // namespace vosfs::rpc::detail

namespace std {
template <>
struct formatter<vosfs::rpc::detail::RpcError> {
public:
    constexpr auto parse(format_parse_context &context) {
        auto it{context.begin()};
        auto end{context.end()};
        if (it == end || *it == '}') {
            return it;
        }
        ++it;
        if (it != end && *it != '}') {
            throw format_error("Invalid format specifier for Error");
        }
        return it;
    }

    auto format(const vosfs::rpc::detail::RpcError &error, auto &context) const noexcept {
        return format_to(context.out(), "{} (error {})", error.message(), error.value());
    }
};
} // namespace std