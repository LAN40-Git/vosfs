#pragma once
#include <cstdint>
#include <string_view>

namespace vosfs {
class Status {
public:
    enum Code : uint32_t {
        kOk = 0,
        kFailed,
        kInvalidArgument,
        kInternal,
        kPermissionDenied,
        kNotLeader,
    };

public:
    explicit Status(uint32_t code)
        : code_(code) {}

public:
    [[nodiscard]]
    auto ok() const -> bool {
        return code_ == kOk;
    }

    [[nodiscard]]
    auto is_failed() const -> bool { return code_ == kFailed; }

    [[nodiscard]]
    auto is_invalid_argument() const -> bool { return code_ == kInvalidArgument; }

    [[nodiscard]]
    auto is_internal() const -> bool { return code_ == kInternal; }

    [[nodiscard]]
    auto is_permission_denied() const -> bool { return code_ == kPermissionDenied; }

    [[nodiscard]]
    auto is_not_leader() const -> bool { return code_ == kNotLeader; }

public:
    [[nodiscard]]
    auto code() const noexcept -> uint8_t {
        return code_;
    }

    [[nodiscard]]
    auto message() const noexcept -> std::string_view {
        switch (code_) {
            case kOk:
                return "ok";
            case kFailed:
                return "failed";
            case kInvalidArgument:
                return "invalid arguement";
            case kInternal:
                return "internal error";
            case kPermissionDenied:
                return "permission denied";
            case kNotLeader:
                return "not leader";
            default:
                return "unknown error";
        }
    }

private:
    uint32_t code_;
};
} // namespace vosfs