#pragma once
#include <cstdint>
#include <string_view>
#include <format>

namespace vosfs::rpc {
class RpcResult {
public:
    enum Status : uint8_t {
        kSuccess = 0,
        kFailed,
        kShutdown,
        kRedirect,
        kNeedShutdown,
        kUnauthenticated,
        kFindServiceTypeFailed,
        kFindMethodTypeFailed,
        kMessageTooLarge,
        kMessageParseFailed,
        kMessageSerializeFailed,
    };

public:
    explicit RpcResult(uint8_t status, uint32_t size)
        : status_(status), size_(size) {}

public:
    [[nodiscard]]
    auto value() const noexcept -> uint8_t { return status_; }

    [[nodiscard]]
    auto size() const noexcept -> uint32_t { return size_; }

    [[nodiscard]]
    auto message() const noexcept -> std::string_view {
        switch (status_) {
        case kSuccess:
            return "Success to handle rpc request.";
        case kFailed:
            return "Failed to handle rpc request.";
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
        case kMessageTooLarge:
            return "Message too large.";
        case kMessageParseFailed:
            return "Failed to parse message.";
        case kMessageSerializeFailed:
            return "Failed to serialize message.";
        default:
            return "Unknown rpc result.";
        }
    }

private:
    uint8_t  status_;
    uint32_t size_;
};

static auto make_result(uint8_t status, uint32_t size = 0) -> RpcResult {
    return RpcResult{status, size};
}
} // namespace vosfs::rpc

namespace std {
template <>
struct formatter<vosfs::rpc::RpcResult> {
public:
    constexpr auto parse(format_parse_context &context) {
        auto it{context.begin()};
        auto end{context.end()};
        if (it == end || *it == '}') {
            return it;
        }
        ++it;
        if (it != end && *it != '}') {
            throw format_error("Invalid format specifier for RpcResult");
        }
        return it;
    }

    auto format(const vosfs::rpc::RpcResult& result, auto &context) const noexcept {
        return format_to(context.out(), "{} (status {})", result.message(), result.value());
    }
};
} // namespace std