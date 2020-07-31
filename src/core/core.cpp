#include <cstdlib>
#ifndef NDEBUG
    #include "vendor.h"
#endif

#include "core.h"
#include <phpext/phpext.h>
#include <boost/asio/io_context.hpp>
#include "url.h"
#include "util.h"

namespace core {
    // 默认上下文（主线程，导出提供其他扩展使用）
    boost::asio::io_context& primary_context() {
        static boost::asio::io_context context;
        return context;
    }
    // 支持上下文（辅线程，导出提供其他扩展使用）
    boost::asio::io_context& support_context() {
        static boost::asio::io_context context;
        return context;
    }
    // 框架初始化
    php::value init(php::parameters& params) {
        return nullptr;
    }
    // 启动
    php::value run(php::parameters& params) {
        primary_context().run();
        return nullptr;
    }
    php::value test(php::parameters& params) {
        url u("protocol://name@pass@host:1234/path?a=b");
        std::cout << u.scheme() << std::endl;
        u.scheme("http").query().set("x", "y");
    
        std::cout << u.str() << std::endl;

        std::cout << static_cast<std::string>(random_string(16)) << std::endl;
        std::cout << static_cast<std::string>($clock) << ": " << static_cast<int>($clock) << std::endl;
        return nullptr;
    }
}
//
extern "C" {
    // PHP 扩展模块入口
    ZEND_DLEXPORT zend_module_entry* get_module() {
        // 构建模块：在离开函数后保留 module 定义
        static php::module_entry module("flame-core", PHP_FLAME_VERSION);
        module
            .declare<core::init>("flame\\init", {
                {"options", php::TYPE_ARRAY, /* byref */false, /* null */true}
            })
            .declare<core::run>("flame\\run")
            .declare<core::test>("flame\\test");

        return module;
    }
}