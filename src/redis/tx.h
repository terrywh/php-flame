#pragma once
#include "../vendor.h"
#include "redis.h"

namespace flame::redis {

    class _connection_lock;
    class tx : public php::class_base {
    public:
        static void declare(php::extension_entry &ext);
        php::value __construct(php::parameters &params); // 私有
        php::value exec(php::parameters& params);
        php::value __call(php::parameters &params);
        // 处理特殊情况的命令
        php::value mget(php::parameters &params);
        php::value hmget(php::parameters &params);
        php::value hgetall(php::parameters &params);
        php::value hscan(php::parameters &params);
        php::value sscan(php::parameters &params);
        php::value zscan(php::parameters &params);
        php::value zrange(php::parameters &params);
        php::value zrevrange(php::parameters &params);
        php::value zrangebyscore(php::parameters &params);
        php::value zrevrangebyscore(php::parameters &params);
        php::value unsubscribe(php::parameters &params);
        php::value punsubscribe(php::parameters &params);
        // 批量
        php::value multi(php::parameters &params);
        // 用于标记不实现的功能
        php::value unimplement(php::parameters &params);
    private:
        std::shared_ptr<_connection_lock> cl_;
        friend class client;
    };
} // namespace flame::redis
