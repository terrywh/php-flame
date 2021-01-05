#include "context.h"
#include "coroutine.hpp"

namespace core {

    std::unique_ptr<context> $ { new context() };
    // 
    context::context()
    // TODO 是否获取自定义的 PHP 配置中种子值？
    : random(reinterpret_cast<std::uintptr_t>(this)) {
        
    }
    // 协程：
    void context::co_sleep(std::chrono::milliseconds ms, coroutine_handler& ch, boost::asio::io_context& io) {
        boost::asio::steady_timer tm(io);
        tm.expires_after(ms);
        tm.async_wait(ch);
    }

    bool context::in_status(status_t s) {
        switch(s) {
        default:
            return env.status & static_cast<int>(s) > 0;
        }
    }
}
