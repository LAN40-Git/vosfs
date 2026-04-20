#pragma once
#include <string>

namespace vosfs::util {
// 默认签发者
constexpr std::string DEFAULT_JWT_ISSUER = "vosfs-issuer";
// 默认签发者密钥
constexpr std::string DEFAULT_JWT_ISSUER_SECRET = "vosfs-secret";
// Token 默认有效时长
constexpr std::size_t DEFAULT_TOKEN_VALID_TIME_IN_SECOND = 3600;
} // namespace vosfs::util