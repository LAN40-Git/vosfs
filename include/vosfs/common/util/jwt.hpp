#pragma once
#include <string>
#include <jwt-cpp/jwt.h>

namespace vosfs::util {
// 默认签发者
constexpr std::string DEFAULT_JWT_ISSUER = "vosfs-issuer";
// 默认签发者密钥
constexpr std::string DEFAULT_JWT_ISSUER_SECRET = "vosfs-secret";
// Token 默认有效时长
constexpr std::size_t DEFAULT_TOKEN_VALID_TIME_IN_SECOND = 3600;

static auto vertify_token(const std::string& token) -> bool {
    try {
        auto decoded = jwt::decode(token);

        auto verifier = jwt::verify()
            .with_issuer(DEFAULT_JWT_ISSUER)
            .allow_algorithm(jwt::algorithm::hs256{DEFAULT_JWT_ISSUER_SECRET});

        verifier.verify(decoded);
    } catch (...) {
        return false;
    }
    return true;
}
} // namespace vosfs::util