#pragma once
#include <assert.h>
#include <hiredis/hiredis.h>
#include <kosio/common/debug.hpp>
#include "vosfs/common/error.hpp"

namespace vosfs::util {
class RedisClient {
private:
    explicit RedisClient(redisContext* context)
        : context_(context) {
        assert(context_);
    }

public:
    ~RedisClient() {
        if (context_) {
            redisFree(context_);
        }
    }

    // Delete copy
    RedisClient(const RedisClient&) = delete;
    auto operator=(const RedisClient&) = delete;

    // Allow move
    RedisClient(RedisClient&& other) noexcept
        : context_(other.context_) {
        other.context_ = nullptr;
    }
    auto operator=(RedisClient&& other) noexcept -> RedisClient& {
        if (&other == this) {
            return *this;
        }

        if (context_) {
            redisFree(context_);
        }

        context_ = other.context_;
        other.context_ = nullptr;
        return *this;
    }

public:
    static auto create(std::string_view ip, int port) -> Result<RedisClient> {
        auto* context = redisConnect(ip.data(), port);
        if (context == nullptr || context->err) {
            if (context) {
                LOG_ERROR("{}", context->err);
                redisFree(context);
            }
            return std::unexpected{make_error(Error::kDatabaseConnectionFailed)};
        }
        return RedisClient{context};
    }

public:
    [[nodiscard]]
    auto get(std::string_view key) const -> Result<std::string> {
        auto* reply = static_cast<redisReply*>(redisCommand(context_, "GET %s", key.data()));
        if (!reply) {
            LOG_ERROR("redis get command failed");
            return std::unexpected{make_error(Error::kDatabaseQueryFailed)};
        }

        if (reply->type == REDIS_REPLY_NIL) {
            freeReplyObject(reply);
            LOG_ERROR("redis key not found: {}", key);
            return std::unexpected{make_error(Error::kDatabaseQueryFailed)};
        }

        if (reply->type != REDIS_REPLY_STRING) {
            freeReplyObject(reply);
            LOG_ERROR("redis get invalid reply type");
            return std::unexpected{make_error(Error::kDatabaseQueryFailed)};
        }

        std::string value(reply->str, reply->len);
        freeReplyObject(reply);
        return value;
    }

    [[nodiscard]]
    auto set(std::string_view key, std::string_view value) const -> Result<void> {
        auto* reply = static_cast<redisReply*>(redisCommand(context_, "SET %s %s", key.data(), value.data()));
        if (!reply) {
            LOG_ERROR("redis set command failed");
            return std::unexpected{make_error(Error::kDatabaseInsertFailed)};
        }

        bool ok = (reply->type == REDIS_REPLY_STATUS) && (strcmp(reply->str, "OK") == 0);
        freeReplyObject(reply);

        if (!ok) {
            LOG_ERROR("redis set failed");
            return std::unexpected{make_error(Error::kDatabaseInsertFailed)};
        }
        return Result<void>{};
    }

    [[nodiscard]]
    auto set_timeout(std::string_view key, std::string value, int timeout) const -> Result<void> {
        auto* reply = static_cast<redisReply*>(redisCommand(context_, "SET %s %s EX %d", key.data(), value.data(), timeout));
        if (!reply) {
            LOG_ERROR("redis set command failed");
            return std::unexpected{make_error(Error::kDatabaseInsertFailed)};
        }

        bool ok = (reply->type == REDIS_REPLY_STATUS) && (strcmp(reply->str, "OK") == 0);
        freeReplyObject(reply);

        if (!ok) {
            LOG_ERROR("redis set failed");
            return std::unexpected{make_error(Error::kDatabaseInsertFailed)};
        }
        return Result<void>{};
    }

    [[nodiscard]]
    auto del(std::string_view key) const -> Result<void> {
        auto* reply = static_cast<redisReply*>(redisCommand(context_, "DEL %s", key.data()));
        if (!reply) {
            LOG_ERROR("redis del command failed");
            return std::unexpected{make_error(Error::kDatabaseDeleteFailed)};
        }

        bool deleted = (reply->type == REDIS_REPLY_INTEGER) && (reply->integer > 0);
        freeReplyObject(reply);

        if (!deleted) {
            LOG_ERROR("redis del failed or key not found");
            return std::unexpected{make_error(Error::kDatabaseDeleteFailed)};
        }
        return Result<void>{};
    }

    [[nodiscard]]
    auto exists(std::string_view key) const -> Result<bool> {
        auto* reply = static_cast<redisReply*>(redisCommand(context_, "EXISTS %s", key.data()));
        if (!reply) {
            LOG_ERROR("redis exists command failed");
            return std::unexpected{make_error(Error::kDatabaseQueryFailed)};
        }

        bool exist = false;
        if (reply->type == REDIS_REPLY_INTEGER) {
            exist = (reply->integer == 1);
        } else {
            LOG_ERROR("redis exists invalid reply type");
            freeReplyObject(reply);
            return std::unexpected{make_error(Error::kDatabaseQueryFailed)};
        }

        freeReplyObject(reply);
        return exist;
    }

private:
    redisContext* context_;
};
} // namespace vosfs::util
