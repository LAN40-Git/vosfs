#pragma once
#include <expected>
#include <string_view>
#include <cstring>
#include <format>
#include <kosio/common/error.hpp>

namespace vosfs {
class Error {
public:
    enum Code {
        // ====== 数据库错误 ======
        kDatabaseConnectionFailed = 0,
        kDatabaseQueryFailed,
        kDatabaseInsertFailed,
        kDatabaseUpdateFailed,
        kDatabaseDeleteFailed,
        // ====== 文件错误 ======
        kFileOpenFailed,
        kFileCreateFailed,
        kFileNotFound,
        kMaxErrorCode
    };

public:
    explicit Error(int code) : code_(code) {}

public:
    [[nodiscard]]
    auto code() const noexcept -> int {
        return code_;
    }

    [[nodiscard]]
    auto message() const noexcept -> std::string_view {
        switch (code_) {
        case kDatabaseConnectionFailed:
            return "database connection failed";
        case kDatabaseQueryFailed:
            return "database query execution failed";
        case kDatabaseInsertFailed:
            return "database insert operation failed";
        case kDatabaseUpdateFailed:
            return "database update operation failed";
        case kDatabaseDeleteFailed:
            return "database delete operation failed";
        default:
            return strerror(code_);
        }
    }

private:
    int code_;
};

template <typename T>
using Result = std::expected<T, Error>;

static auto make_error(int code) -> Error {
    return Error{code};
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
        return format_to(context.out(), "{} (error {})", error.message(), error.code());
    }
};
} // namespace std