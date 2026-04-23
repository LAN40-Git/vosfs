#pragma once
#include <cstdint>
#include <string_view>

namespace vosfs::auth {
class Status {
public:
    enum Code : uint32_t {
        kOk = 0,
        kInvalidArgument,
        kInternal,
        kPermissionDenied,
    };

public:
    explicit Status(uint32_t code)
        : code_(code) {}

public:
    [[nodiscard]]
    auto ok() const -> bool {
        return code_ == kOk;
    }

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
            case kInvalidArgument:
                return "invalid arguement";
            case kInternal:
                return "internal error";
            case kPermissionDenied:
                return "permission denied";
            default:
                return "unknown error";
        }
    }

private:
    uint32_t code_;
};
} // namespace vosfs::auth