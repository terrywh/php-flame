
#include "core.h"
#include "worker.h"
#include "url.h"
#include "util.h"
#include "logger.h"

#include <phpext/phpext.h>
#include <boost/asio/io_context.hpp>

namespace flame { namespace core {

    static boost::asio::io_context master_context_;
    // 默认上下文（主线程，导出提供其他扩展使用）
    boost::asio::io_context& master_context() {
        return master_context_;
    }
    static boost::asio::io_context worker_context_;
    // 支持上下文（辅线程，导出提供其他扩展使用）
    boost::asio::io_context& worker_context() {
        return worker_context_;
    }
    // 框架初始化
    php::value init(php::parameters& params) {
        return nullptr;
    }
    static worker worker_;
    // 启动
    php::value run(php::parameters& params) {
        worker_.start();
        master_context().run();
        return nullptr;
    }

    php::value test(php::parameters& params) {
        // url u("protocol://name@pass@host:1234/path?a=b");
        // std::cout << u.scheme() << " / " << u << std::endl;
        // u.scheme("http").query().set("x", "y");
    
        // std::cout << u.str(false) << " / " << u.str(true) << std::endl;

        // std::cout << static_cast<std::string>(random_string(16)) << std::endl;
        // std::cout << static_cast<std::string>($clock) << ": " << static_cast<int>($clock) << std::endl;
        return nullptr;
    }

    void stop() {
        worker_.stop();
    }
}}
//
extern "C" {
    // PHP 扩展模块入口
    ZEND_DLEXPORT zend_module_entry* get_module() {
        // 构建模块：在离开函数后保留 module 定义
        static php::module_entry module("flame-core", PHP_FLAME_CORE_VERSION);
        module
            .on(php::module_shutdown([] (php::module_entry& module) {
                flame::core::stop();
            }))
            .declare<flame::core::init>("flame\\init", {
                {"name", php::TYPE_STRING, /* byref */false, /* null */false},
                {"options", php::TYPE_ARRAY, /* byref */false, /* null */true},
            })
            .declare<flame::core::run>("flame\\run")
            .declare<flame::core::test>("flame\\test");
        // 日志模块生命
        flame::core::logger::declare(module);
        return module;
    }
}