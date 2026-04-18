#pragma once
#include <string_view>

namespace vosfs::rpc{
enum class StatusCode : unsigned char {
    kUnknown = 0,
    kOk,
    kCancelled,
    kNotFound,
    kInvalidArgument,
    kDeadlineExceeded,
    kPermissionDenied,
    kUnavailable
};
} // namespace vosfs::rpc