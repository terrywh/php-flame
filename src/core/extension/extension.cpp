#include "../context.h"
#include "core.h"
#include "time.h"
#include "util.h"
#include "log.h"

extern "C" {
    // PHP 扩展模块入口
    ZEND_DLEXPORT zend_module_entry* get_module() {
        // 构建模块：在离开函数后保留 module 定义
        static php::module_entry entry("flame-core", "v0.18.0");
        entry
            .declare<core::extension::core>();
        if(core::$context->is_main())
            return entry;

        // 仅在工作进程生成注册相关函数功能
        entry
            .declare<core::extension::time>()
            .declare<core::extension::util>()
            .declare<core::extension::log>();
        return entry;
    }
}