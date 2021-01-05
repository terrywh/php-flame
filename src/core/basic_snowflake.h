#ifndef CORE_SNOWFLAKE_H
#define CORE_SNOWFLAKE_H

#include "context.h"
#include <cstdint>

namespace core { 

    class basic_snowflake {
    public:
        static const int64_t default_epoch = 1288834974657;
        explicit basic_snowflake(int16_t node = $context->env.pid, int64_t epoch = default_epoch);
        std::int64_t next_id();

    protected:
        std::int64_t time_:42;
        std::int64_t node_:10;
        std::int64_t seq_:12;
        std::int64_t epoch_;
    };
}
#endif // CORE_SNOWFLAKE_H
