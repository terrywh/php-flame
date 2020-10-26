#ifndef PHP_FLAME_CORE_CONTROLLER_H
#define PHP_FLAME_CORE_CONTROLLER_H

#include "coroutine.hpp"

#include <phpext.h>
#include <boost/core/null_deleter.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/process/environment.hpp>
#include <boost/signals2.hpp>
#include <memory>

#define PHP_FLAME_CORE_VERSION "0.18.0" // 模块版本

namespace flame { namespace core {

    class context;
    extern std::unique_ptr<context> $;

    class context {
    public:
        // 默认上下文（主线程，导出提供其他扩展使用）
        boost::asio::io_context io_m;
        // 支持上下文（辅线程，导出提供其他扩展使用）
        boost::asio::io_context io_w;
        // 当前进程的环境变量
        boost::process::environment env;
        // 初始化选项
        php::value* opt;
        // 事件：框架初始化
        boost::signals2::signal<void ()> on_flame_init;
    public:
        context();

        // 协程辅助
        void co_sleep(std::chrono::milliseconds ms, coroutine_handler& ch, boost::asio::io_context& io = $->io_m);
        // 事件辅助
        template <typename Handler>
        void ev_after(std::chrono::milliseconds ms, Handler&& cb, boost::asio::io_context& io = $->io_m) {
            auto tm = std::make_unique<boost::asio::steady_timer>(io);
            tm->expires_after(ms);
            tm->async_wait([tm = std::move(tm), cb = std::move(cb)] (const boost::system::error_code& error) {
                cb();
            });
        }
    };

}}

#endif // PHP_FLAME_CORE_CONTROLLER_H
