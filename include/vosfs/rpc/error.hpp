#pragma once
#include "vosfs/common/error.hpp"
#include <string_view>
#include <cstdint>

namespace vosfs::rpc {
class RpcError {
public:
    enum ErrorCode : uint8_t {
        kSuccess = 0,
        kShutdown,
        kRedirect,
        kNeedShutdown,
        kUnauthenticated,
        kFindServiceTypeFailed,
        kFindMethodTypeFailed,
        kGetRespPayloadFailed,
        kMessageParseFailed,
        kMessageSerializeFailed,
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
            case kShutdown:
                return "Normal shutdown.";
                case kRedirect:
                return "Need to redirect to leader.";
            case kNeedShutdown:
                return "Need to send the shutdown rp.";
            case kUnauthenticated:
                return "Failed to handle unauthenticated rpc request.";
            case kFindServiceTypeFailed:
                return "Failed to find service type.";
            case kFindMethodTypeFailed:
                return "Failed to find method type.";
            case kMessageParseFailed:
                return "Failed to parse message.";
            case kMessageSerializeFailed:
                return "Failed to serialize message.";
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

} // namespace vosfs::rpc

namespace std {
template <>
struct formatter<vosfs::rpc::RpcError> {
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

    auto format(const vosfs::rpc::RpcError &error, auto &context) const noexcept {
        return format_to(context.out(), "{} (error {})", error.message(), error.value());
    }
};
} // namespace std
