#include "basic_snowflake.h"
#include "context.h"

namespace core { 
    //
    basic_snowflake::basic_snowflake(int16_t node, int64_t epoch)
    : parts_ { 0, node % 1024, 0 }
    , epoch_(epoch) {
        std::uniform_int_distribution<int64_t> dist(0, 1 << 11);
        parts_.seq = dist($context->random) & 0x0fffl;
    }
  
    int64_t basic_snowflake::next_id() {
        parts_.time = static_cast<int64_t>(clock::get_const_instance()) - epoch_;
        ++parts_.seq; // 非单线程环境可能重复
        return flake_;
    }
}
