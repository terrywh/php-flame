#include <phpext.h>
#include "core.h"
#include "time.h"
#include "util.h"

extern "C" {
    // PHP 扩展模块入口
    ZEND_DLEXPORT zend_module_entry* get_module() {
        // 构建模块：在离开函数后保留 module 定义
        static php::module_entry entry("flame-core", "v0.18.0");
        
        entry
            - php::module_shutdown([] (php::module_entry& entry) {
                // if(php::has_error()) $context->env.status |= context::STATUS_ERROR;
            });
        
        entry
            .declare<core::extension::core>()
            .declare<core::extension::time>()
            .declare<core::extension::util>();
        
        return entry;
    }
}