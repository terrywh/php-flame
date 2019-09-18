#pragma once
#include "../../vendor.h"
#include "../coroutine.h"
#include "redis.h"

namespace flame::redis {

    enum class reply_type {
        SIMPLE        = 0x0000,  // 默认返回处理
        ASSOC_ARRAY_1 = 0x0001,  // 返回数据多项按 KEY VAL 生成关联数组
        ASSOC_ARRAY_2 = 0x0002,  // 第一层普通数组, 第二层关联数组 (HSCAN/ZSCAN)
        ASSOC_ARRAY_3 = 0x0010,  // 返回数据多项按 VAL KEY 生成关联数组
        COMBINE_1     = 0x0004,  // 与参数结合生成关联数组
        COMBINE_2     = 0x0008,  // 同上, 但偏移错位 1 个参数
        EXEC          = 0x0100, // 事务特殊处理方式（需要结合上面几种方式）
        PIPE          = 0x0200, // 与事务形式相同
    };

    class _connection_base {
    public:
        virtual std::shared_ptr<redisContext> acquire(coroutine_handler &ch) = 0;
        php::value reply2value(redisReply *rp, php::array &argv, reply_type rt);
        std::string format(php::string& name, php::array& argv);
    };
} // namespace flame::mysql
