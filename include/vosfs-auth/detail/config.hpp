#pragma once
#include <cstdint>

namespace vosfs::auth::detail {
// 用户默认配额（100 GB）
constexpr uint64_t DEFAULT_USER_QUOTA_BYTES = 100ULL * 1024 * 1024 * 1024;
// 默认管理员密钥
constexpr std::string_view DEFAULT_ADMIN_SECRET = "123456";
} // namespace vosfs::auth::detail