#include "basic_snowflake.h"
#include "context.h"

namespace core { 
    //
    basic_snowflake::basic_snowflake(int16_t node, int64_t epoch)
    : node_(node % 1024)
    , epoch_(epoch) {
        std::uniform_int_distribution<int> dist(0, 1 << 11);
        seq_ = dist($context->random);
    }
  
    int64_t basic_snowflake::next_id() {
        time_ = static_cast<int64_t>(clock::get_const_instance()) - epoch_;
        ++seq_; // 单线程环境
        return *reinterpret_cast<int64_t*>(this);
    }
}
