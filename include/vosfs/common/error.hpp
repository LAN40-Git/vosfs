#pragma once
#include <expected>
#include <string_view>
#include <cstring>
#include <format>
#include <kosio/common/error.hpp>

namespace vosfs {
class Error {
public:
    enum ErrorCode {
        kUnknown = 8000,
        kProtoSerializeFailed,
        kProtoParseFailed,
        kTruncateFailed,
        kRecoverFailed,
        kCreateRocksDBEngineFailed,
        kCreatePeerFailed,
        kQueueShutdown,
        kConsumerShutdown,
        kConsumerRunning,
    };

public:
    explicit Error(int error_code) : error_code_(error_code) {}

public:
    [[nodiscard]]
    auto message() const noexcept -> std::string_view {
        switch (error_code_) {
            case kUnknown:
                return "Unknown error.";
            case kProtoSerializeFailed:
                return "Failed to serialize proto.";
            case kProtoParseFailed:
                return "Failed to parse proto.";
            case kTruncateFailed:
                return "Failed to truncate data.";
            case kRecoverFailed:
                return "Failed to recover data.";
            case kCreateRocksDBEngineFailed:
                return "Failed to create rocksdb engine.";
            case kCreatePeerFailed:
                return "Failed to create peer.";
            case kQueueShutdown:
                return "Shutdown queue.";
            case kConsumerShutdown:
                return "Consumer has shutdown.";
            case kConsumerRunning:
                return "Consumer is running.";
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