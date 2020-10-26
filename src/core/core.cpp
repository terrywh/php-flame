#include "context.hpp"
#include "function.h"
#include "logger.hpp"

extern "C" {
    // PHP 扩展模块入口
    ZEND_DLEXPORT zend_module_entry* get_module() {
        // 构建模块：在离开函数后保留 module 定义
        static php::module_entry module("flame-core", PHP_FLAME_CORE_VERSION);
        // 核心函数
        flame::core::function::declare(module);
        // 日志
        flame::core::logger::declare(module);
        return module;
    }
}