#include "vendor.h"
#include "http.h"
#include <phpext/phpext.h>

namespace http {
    //
    php::value test(php::parameters& params) {
        return nullptr;
    }
}
//
extern "C" {
    // PHP 扩展模块入口
    ZEND_DLEXPORT zend_module_entry* get_module() {
        // 构建模块：在离开函数后保留 module 定义
        static php::module_entry module("flame-http", PHP_FLAME_HTTP_VERSION);
        module
            .require("flame-core")
            .declare<http::test>("flame\\http\\test");

        return module;
    }
}