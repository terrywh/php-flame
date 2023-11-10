#ifndef CORE_SNOWFLAKE_H
#define CORE_SNOWFLAKE_H

#include "vendor.h"

namespace core { 

    class basic_snowflake {
    public:
        static const int64_t default_epoch = 1288834974657;
        explicit basic_snowflake(int16_t node, int64_t epoch = default_epoch);
        std::int64_t next_id();

    protected:
        union {
            struct {
                std::int64_t  time:42;
                std::int64_t  node:10;
                std::int64_t   seq:12;
            } parts_;
            int64_t flake_;
        };
        std::int64_t epoch_;
    };
}
#endif // CORE_SNOWFLAKE_H
