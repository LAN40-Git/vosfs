#pragma once
#include <expected>
#include <string_view>
#include <cstring>
#include <format>

namespace vosfs {
class Error {
public:
    enum ErrorCode {
        kUnknown = 8000,
        kInvalidAddress,
        kConnectToServerFailed,
        kSendRpcRequestFailed,
        kParseRpcMessageFailed,
        kBindFailed,
        kConnectionShutdown,
        kQueueEmpty,
        kQueueShutdown,
        kMessageParseFailed,
        kMessageSerializeFailed,
        kProviderIsRunning,
        kProviderHasShutdown,
        kConsumerHasShutdown,
        kStopListeningFailed,
        kNeedRedirect,
    };

public:
    explicit Error(int error_code) : error_code_(error_code) {}

public:
    [[nodiscard]]
    auto message() const noexcept -> std::string_view {
        switch (error_code_) {
            case kUnknown:
                return "Unknown error.";
            case kInvalidAddress:
                return "Invalid address.";
            case kConnectToServerFailed:
                return "Failed to connect to server.";
            case kSendRpcRequestFailed:
                return "Failed to send rpc request.";
            case kParseRpcMessageFailed:
                return "Failed to parse rpc message.";
            case kBindFailed:
                return "Failed to bind to address.";
            case kConnectionShutdown:
                return "Connection shutdown.";
            case kQueueEmpty:
                return "Empty queue.";
            case kQueueShutdown:
                return "Shutdown queue.";
            case kMessageParseFailed:
                return "Failed to parse message.";
            case kMessageSerializeFailed:
                return "Failed to serialize message.";
            case kProviderIsRunning:
                return "The provider is running.";
            case kProviderHasShutdown:
                return "The provider has shutdown.";
            case kConsumerHasShutdown:
                return "The consumer has shutdown.";
            case kStopListeningFailed:
                return "Failed to stop listening.";
            case kNeedRedirect:
                return "Need to redirect to leader.";
            default:
                return strerror(error_code_);
        }
    }

    [[nodiscard]]
    auto value() const noexcept -> int {
        return error_code_;
    }

private:
    int error_code_;
};

template <typename T>
using Result = std::expected<T, Error>;

static auto make_error(int error_code) -> Error {
    return Error{error_code};
}
} // namespace vosfs

namespace std {
template <>
struct formatter<vosfs::Error> {
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

    auto format(const vosfs::Error &error, auto &context) const noexcept {
        return format_to(context.out(), "{} (error {})", error.message(), error.value());
    }
};
} // namespace std