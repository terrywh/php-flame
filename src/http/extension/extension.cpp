#include <phpext.h>
//
extern "C" {
    // PHP 扩展模块入口
    ZEND_DLEXPORT zend_module_entry* get_module() {
        // 构建模块：在离开函数后保留 module 定义
        static php::module_entry module("flame-http", "v0.18.0");
        module
            - php::require("flame-core");

        return module;
    }
}