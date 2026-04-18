#include <iostream>
#include <hiredis/hiredis.h>

int main() {
    // 连接 Redis
    redisContext* ctx = redisConnect("127.0.0.1", 6379);
    if (ctx == nullptr || ctx->err) {
        std::cerr << "Redis 连接失败！" << std::endl;
        return -1;
    }

    std::cout << "Redis 连接成功！" << std::endl;

    // 测试 set
    redisCommand(ctx, "SET token_123 user_001");

    // 测试 get
    redisReply* reply = (redisReply*)redisCommand(ctx, "GET token_123");
    if (reply->type == REDIS_REPLY_STRING) {
        std::cout << "获取到值：" << reply->str << std::endl;
    }

    // 释放
    freeReplyObject(reply);
    redisFree(ctx);

    return 0;
}