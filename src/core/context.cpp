#include "context.h"

namespace core {

    std::unique_ptr<context> $context { new context() };
    // 
    context::context()
    // TODO 是否获取自定义的 PHP 配置中种子值？
    : random(reinterpret_cast<std::uintptr_t>(this)) {
        
    }
    //
    bool context::in_status(status_t s) {
        switch(s) {
        default:
            return env.status & static_cast<int>(s) > 0;
        }
    }
}
